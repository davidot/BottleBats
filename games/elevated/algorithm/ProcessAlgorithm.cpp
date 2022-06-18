#include "ProcessAlgorithm.h"
#include "../../../util/Assertions.h"
#include <algorithm>
#include <charconv>
#include <numeric>
#include <set>
#include <sstream>

namespace Elevated {

ProcessAlgorithm::ProcessAlgorithm(std::vector<std::string> command, InfoLevel info_level, util::SubProcess::StderrState stderr_state, std::string working_directory)
    : m_command(std::move(command))
    , m_info_level(info_level)
    , m_stderr_handling(stderr_state)
    , m_working_directory(std::move(working_directory))
{
    ASSERT(!m_command.empty());
    ASSERT(m_info_level == InfoLevel::Low);
}

ProcessAlgorithm::~ProcessAlgorithm()
{
    if (m_process) {
        m_process->writeToWithTimeout("stop\n", 50);
        std::string fake_line;
        m_process->readLineWithTimeout(fake_line, 50);
    }
}

void ProcessAlgorithm::write_building(BuildingGenerationResult const& building, std::ostringstream& stream)
{
    stream << "building " << building.blueprint().reachable_per_group.size()
            << ' ' << building.blueprint().elevators.size()
            << '\n';

    GroupID group_id {0};

    for (auto& reachable_floors : building.blueprint().reachable_per_group) {
        stream << "group " << group_id << ' ' << reachable_floors.size() << ' ';

        bool first = true;
        for (auto& floor : std::set<Height>(reachable_floors.begin(), reachable_floors.end())) {
            if (!first)
                stream << ',';
            stream << floor;
            first = false;
        }

        stream << '\n';
        ++group_id;
    }

    ElevatorID elevator_id {0};

    for (auto& elevator : building.blueprint().elevators) {
        stream << "elevator " << elevator_id << ' '
                << elevator.group << ' '
                << elevator.speed << ' '
                << elevator.max_capacity << ' '
                << ElevatorState::door_opening_time << ' '
                << ElevatorState::door_closing_time << '\n';

        ++elevator_id;
    }
}

ElevatedAlgorithm::ScenarioAccepted ProcessAlgorithm::accept_scenario_description(BuildingGenerationResult const& building)
{
    m_process = util::SubProcess::create(m_command, m_stderr_handling, m_working_directory);
    if (!m_process) {
        return ScenarioAccepted::failed({ "Failed to start process", make_command_string() });
    }

    std::ostringstream message;

    std::string info_level;
    switch (m_info_level) {
    case InfoLevel::Full:
        info_level = "full";
        break;
    case InfoLevel::High:
        info_level = "high";
        break;
    case InfoLevel::Low:
        info_level = "low";
        break;
    case InfoLevel::Minimal:
        info_level = "min";
        break;
    }

    message << "elevated\n"
            << "setting info " << info_level << '\n'
            << "setting commands basic\n"
            << "setting capacity " << (building.has_infinite_capacity() ? "off" : "on") << '\n';

    write_building(building, message);
    message << "done\n";

    size_t start_up_time;
    auto val = std::move(*message.rdbuf()).str();
    auto result = m_process->sendAndWaitForResponse(val, 1500, &start_up_time);
    if (!result.has_value())
        return ScenarioAccepted::failed({ "Process failed to respond to setup, command: ", make_command_string() });

    if (*result == "ready\n")
        return ScenarioAccepted::accepted();

    if (result->starts_with("reject"))
        return ScenarioAccepted::rejected({*result});

    return ScenarioAccepted::failed( {"Process gave non reject/ready result, got:", *result} );
}

void ProcessAlgorithm::write_elevator_base(const ElevatorState& elevator, std::ostringstream& stream) const
{
    stream << elevator.id << ' '
           << elevator.group_id << ' '
           << elevator.height();
    std::set<Height> targets;
    std::transform(elevator.passengers().begin(), elevator.passengers().end(),
        std::inserter(targets, targets.begin()), [](ElevatorState::TravellingPassenger const& passenger){
            return passenger.to;
        });

    stream << ' ' << targets.size() << ' ';

    bool first = true;
    for (Height target : targets) {
        if (!first)
            stream << ',';
        stream << target;
        first = false;
    }

    if (targets.empty())
        stream << '-';
}

void ProcessAlgorithm::write_elevator_closed(BuildingState const& building, ElevatorID elevator_id, std::ostringstream& stream) const
{
    auto& elevator = building.elevator(elevator_id);
    write_elevator_base(elevator, stream);

    ASSERT(m_info_level == InfoLevel::Low);

    stream << " still-waiting ";
    auto const& queue = building.passengers_at(elevator.height());
    auto [up, down] = std::accumulate(queue.begin(), queue.end(), std::pair<bool, bool>{false, false},
        [&](auto acc, Passenger const& entry){
            ASSERT(entry.from == elevator.height());
            if (entry.group != elevator.group_id)
                return acc;
            bool request_up = entry.to > entry.from;
            return std::pair<bool, bool>{
                acc.first || request_up,
                acc.second || !request_up,
            };
        });

    if (up && down)
        stream << "up,down";
    else if (up)
        stream << "up";
    else if (down)
        stream << "down";
    else
        stream << "-";

    // FIXME: Give more info for higher levels.
}

void ProcessAlgorithm::write_new_request(Passenger const& request, std::ostringstream& stream) const
{
    ASSERT(m_info_level == InfoLevel::Low);

    bool going_up = request.to > request.from;

    stream << request.from << ' ' << request.group << ' ';
    if (going_up)
        stream << "up";
    else
        stream << "down";
}

bool ProcessAlgorithm::should_write_new_request(BuildingState const& building, Height target, size_t index)
{
    ASSERT(m_info_level == InfoLevel::Low);
    auto& queue = building.passengers_at(target);
    ASSERT(queue.size() > index);
    auto& request = queue[index];
    bool going_up = request.to > request.from;

    return std::none_of(queue.begin(), std::next(queue.begin(), index), [&](Passenger const& item) {
        if (item.group != request.group)
            return false;

        bool also_going_up = item.to > item.from;
        return going_up == also_going_up;
    });
}

static std::optional<uint64_t> parse_unsigned(std::string_view view) {
    uint64_t val;
    auto [ptr, ec] { std::from_chars(view.data(), view.data() + view.size(), val) };
    if (ec == std::errc{}) {
        if (ptr == view.data() + view.size())
            return val;
    }
    return {};
}

std::vector<AlgorithmResponse> ProcessAlgorithm::on_inputs(Time at, BuildingState const& building, std::vector<AlgorithmInput> inputs)
{
    std::ostringstream message;
    size_t events = 0;
    for (auto& input : inputs) {
        switch (input.type()) {
        case AlgorithmInput::Type::NewRequestMade:
            if (should_write_new_request(building, input.request_height(), input.request_index())) {
                message << "request ";
                write_new_request(input.request(building), message);
            } else {
                continue;
            }
            break;
        case AlgorithmInput::Type::ElevatorClosedDoors:
            message << "closed ";
            write_elevator_closed(building, input.elevator_id(), message);
            break;
        case AlgorithmInput::Type::TimerFired:
            message << "timer";
            break;
        }
        events++;
        message << '\n';
    }

    if (!events)
        return {};

    message << "done\n";

    {
        std::ostringstream temp;
        temp << "events " << at << ' ' << events << '\n';
        temp << message.str();
        message = std::move(temp);
    }

    size_t time_taken;
    size_t time_left = 500;
    auto result = m_process->sendAndWaitForResponse(message.str(), time_left, &time_taken);
    if (!result.has_value())
        return { AlgorithmResponse::algorithm_failed({ "Process failed to respond to messages, command: ", make_command_string(), "input: ", message.str() }) };

    std::vector<AlgorithmResponse> responses;

    std::string line = result.value();

    while (line != "done\n") {
        if (line.starts_with("move ")) {
            std::string_view view = line;
            ASSERT(line[line.size() - 1] == '\n');
            view.remove_suffix(1);
            view.remove_prefix(5);

            auto middle = view.find(' ');
            auto elevator_id_or_none = parse_unsigned(view.substr(0, middle));
            if (!elevator_id_or_none.has_value())
                return { AlgorithmResponse::algorithm_failed({ "Process sent move but did not have elevator id:", line }) };
            if (elevator_id_or_none.value() >= building.num_elevators())
                return { AlgorithmResponse::algorithm_misbehaved({ "Process sent move with incorrect elevator id:", line }) };

            auto second_part = view.find(' ', middle + 1);

            std::string_view target_view;

            if (second_part == std::string_view::npos)
                target_view = view.substr(middle + 1);
            else
                target_view = view.substr(middle + 1, second_part - middle - 1);

            auto target_or_none = parse_unsigned(target_view);
            if (!target_or_none.has_value())
                return { AlgorithmResponse::algorithm_failed({ "Process sent move but did not have (valid) target height:", line }) };

            if (second_part != std::string_view::npos) {
                auto filter = view.substr(second_part + 1);
                if (filter == "up")
                    m_filters[elevator_id_or_none.value()] = PassengerFilter::UpOnly;
                else if (filter == "down")
                    m_filters[elevator_id_or_none.value()] = PassengerFilter::DownOnly;
                else
                    return { AlgorithmResponse::algorithm_failed({ "Process sent move but did not have (valid) filter:", line }) };
            } else {
                m_filters.erase(elevator_id_or_none.value());
            }

            responses.push_back(AlgorithmResponse::move_elevator_to(elevator_id_or_none.value(), target_or_none.value()));
        } else if (line.starts_with("set-timer ")){
            std::string_view view = line;
            ASSERT(line[line.size() - 1] == '\n');
            view.remove_suffix(1);
            view.remove_prefix(10);

            auto time_or_none = parse_unsigned(view);
            if (!time_or_none.has_value())
                return { AlgorithmResponse::algorithm_failed({ "Process sent set-timer but did not have (just) time:", line }) };

            responses.push_back(AlgorithmResponse::set_timer_at(time_or_none.value()));
        } else {
            return { AlgorithmResponse::algorithm_misbehaved({ "Process gave invalid command: ", line, "for input: ", message.str() }) };
        }

        if (!m_process->readLineWithTimeout(line, 150))
            return { AlgorithmResponse::algorithm_failed({ "Process failed to respond to messages, command: ", make_command_string(), "input: ", message.str() }) };
    }

    return responses;
}



std::string ProcessAlgorithm::make_command_string() const
{
    std::string command_value;
    for (auto& part : m_command)
        command_value += part + " ";
    return command_value;
}

static bool up_only(Passenger const& passenger) {
    return passenger.to > passenger.from;
}

static bool down_only(Passenger const& passenger) {
    return passenger.to < passenger.from;
}

std::optional<ElevatorState::PassengerCallback> ProcessAlgorithm::on_doors_open(Time, ElevatorID id, BuildingState const&)
{
    if (auto it_or_end = m_filters.find(id); it_or_end != m_filters.end()) {
        if (it_or_end->second == PassengerFilter::UpOnly) {
            return up_only;
        } else {
            ASSERT(it_or_end->second == PassengerFilter::DownOnly);
            return down_only;
        }
    }
    return {};
}

}

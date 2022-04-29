#include "ProcessAlgorithm.h"
#include "../../../util/Assertions.h"
#include <set>
#include <sstream>

namespace Elevated {

ProcessAlgorithm::ProcessAlgorithm(std::vector<std::string> command, InfoLevel info_level)
    : m_command(std::move(command))
    , m_info_level(info_level)
{
    ASSERT(!m_command.empty());
}

ProcessAlgorithm::~ProcessAlgorithm()
{
    if (m_process)
        m_process->writeToWithTimeout("stop", 10);
}

void ProcessAlgorithm::write_building(BuildingGenerationResult const& building, std::ostringstream& stream)
{
    stream << "building " << building.blueprint().reachable_per_group.size()
            << ' ' << building.blueprint().elevators.size()
            << '\n';

    GroupID group_id {0};

    for (auto& reachable_floors : building.blueprint().reachable_per_group) {
        stream << "group " << group_id << ' ' << reachable_floors.size();

        for (auto& floor : std::set<Height>(reachable_floors.begin(), reachable_floors.end()))
            stream << ' ' << floor;

        stream << '\n';
        ++group_id;
    }

    ElevatorID elevator_id {0};

    for (auto& elevator : building.blueprint().elevators) {
        stream << "elevator " << elevator_id << ' '
                << elevator.group << ' '
                << ElevatorState::speed << ' '
                << elevator.max_capacity << ' '
                << ElevatorState::door_opening_time << ' '
                << ElevatorState::door_closing_time << '\n';

        ++elevator_id;
    }
}

ElevatedAlgorithm::ScenarioAccepted ProcessAlgorithm::accept_scenario_description(BuildingGenerationResult const& building)
{
    m_process = util::SubProcess::create(m_command);
    if (!m_process) {
        std::vector<std::string> messages {"Failed to start process: "};
        messages.reserve(m_command.size() + 1);
        messages.insert(messages.end(), m_command.begin(), m_command.end());
        return ScenarioAccepted::failed( std::move(messages) );
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

    auto val = std::move(*message.rdbuf()).str();
    auto result = m_process->sendAndWaitForResponse(val, 1500);
    if (!result.has_value()) {
        std::vector<std::string> messages {"Process failed to respond to setup, command: "};
        messages.reserve(m_command.size() + 1);
        messages.insert(messages.end(), m_command.begin(), m_command.end());
        return ScenarioAccepted::failed( std::move(messages) );
    }

    if (*result == "ready\n")
        return ScenarioAccepted::accepted();

    if (result->starts_with("reject"))
        return ScenarioAccepted::rejected({*result});

    return ScenarioAccepted::failed( {"Process gave non reject/ready result, got:", *result} );
}

std::vector<AlgorithmResponse> ProcessAlgorithm::on_inputs(Time, BuildingState const&, std::vector<AlgorithmInput>)
{
    return {};
}


}

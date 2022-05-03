#include "Simulation.h"
#include "../../util/Assertions.h"
#include "../../util/Deferred.h"
#include <algorithm>
#include <iostream>
#include <string>

namespace Elevated {

Simulation::Simulation(std::unique_ptr<ScenarioGenerator> generator, std::unique_ptr<ElevatedAlgorithm> algorithm)
    : m_generator(std::move(generator))
    , m_algorithm(std::move(algorithm))
{
    ASSERT(m_generator);
    ASSERT(m_algorithm);
}

bool Simulation::setup_for_run()
{
    auto building_result = m_generator->generate_building();
    std::sort(building_result.blueprint().elevators.begin(), building_result.blueprint().elevators.end(), [](auto const& lhs, auto const& rhs) {
        return lhs.group < rhs.group;
    });
    if (building_result.has_error()) {
        result->type = SimulatorResult::Type::GenerationFailed;
        result->output_messages = building_result.errors();
        return false;
    }

    auto accepted = m_algorithm->accept_scenario_description(building_result);

    if (accepted.type != ElevatedAlgorithm::ScenarioAccepted::Type::Accepted) {
        result->type = accepted.type == ElevatedAlgorithm::ScenarioAccepted::Type::Rejected ? SimulatorResult::Type::AlgorithmRejected : SimulatorResult::Type::AlgorithmFailed;
        result->output_messages = std::move(accepted.messages);
        return false;
    }

    m_event_distributor.on_initial_building(building_result.blueprint());

    m_building = BuildingState { building_result.extract_blueprint(), &m_event_distributor };

    return true;
}

std::optional<Time> min_time(NextRequests requests, std::optional<Time> building_time, std::optional<Time> timer_time) {
    std::optional<Time> least;
    if (requests.type == NextRequests::Type::At)
        least = requests.next_request_time;

    if (building_time.has_value() && (!least.has_value() || building_time < least))
        least = building_time;

    if (timer_time.has_value() && (!least.has_value() || timer_time < least))
        return timer_time;

    return least;
}

SimulatorResult Simulation::run()
{
    if (result.has_value() || !m_algorithm) {
        std::cerr << "Simulation was ran twice?\n";
        return *result;
    }

    Deferred remove_algorithm ([&]{
        m_algorithm.reset();
    });

    result = SimulatorResult{};

    if (!setup_for_run())
        return *result;

    bool running = true;
    NextRequests next_request_time = m_generator->next_requests_at();
    Time last_requests = 0;
    std::optional<Time> next_timer = 0;

    Time extra_time_after_last_request = 100000;

    while (running) {
        if (next_request_time.type == NextRequests::Type::Done && m_building.passengers_done())
            break;

        auto next_building_event = m_building.next_event_at();

        auto next_time_or_none = min_time(next_request_time, next_building_event, next_timer);
        if (!next_time_or_none.has_value()) {
            running = false;
            result->type = SimulatorResult::Type::NoNextEvent;
            return *result;
        }

        auto running_until = *next_time_or_none;

        if (next_request_time.type != NextRequests::Type::At && running_until > last_requests + extra_time_after_last_request) {
            result->type = SimulatorResult::Type::FailedToResolveAllRequests;
            return *result;
        }

        auto elevators_closed = m_building.update_until(running_until);
        ASSERT(m_generator->next_requests_at() > running_until || next_request_time == running_until);

        std::vector<AlgorithmInput> inputs;

        if (next_request_time == running_until) {
            last_requests = running_until;
            auto new_requests = m_generator->requests_at(running_until);
            inputs.reserve(new_requests.size() + elevators_closed.size());
            for (auto& new_request : new_requests) {
                auto queue_index = m_building.add_request(new_request);
                if (!queue_index.has_value()) {
                    result->type = SimulatorResult::Type::RequestGenerationFailed;
                    result->output_messages.emplace_back("Failing because: Added request from: " + std::to_string(new_request.from) + " to " + std::to_string(new_request.to) + " within group " + std::to_string(new_request.group));
                    return *result;
                }
                inputs.push_back(AlgorithmInput::new_request(new_request.from, queue_index.value()));
            }
        }

        inputs.reserve(elevators_closed.size());
        std::transform(elevators_closed.begin(), elevators_closed.end(), std::back_inserter(inputs), [](ElevatorID id){
            return AlgorithmInput::elevator_closed_doors(id);
        });

        if (next_timer == running_until)
            inputs.push_back(AlgorithmInput::timer_fired());

        next_timer.reset();

        if (!inputs.empty()) {
            auto commands = m_algorithm->on_inputs(running_until, m_building, inputs);

            for (auto const& command : commands) {
                if (command.type() == AlgorithmResponse::Type::MoveElevator) {
                    if (!m_building.send_elevator(command.elevator_to_move(), command.elevator_target())) {
                        result->type = SimulatorResult::Type::AlgorithmMisbehaved;
                        result->output_messages.emplace_back("Failing because: Command to move elevator: " + std::to_string(command.elevator_to_move()) + " to " + std::to_string(command.elevator_target()));
                        return *result;
                    }
                } else if (command.type() == AlgorithmResponse::Type::SetTimer) {
                    next_timer = command.timer_should_fire_at();
                    if (next_timer <= running_until) {
                        result->type = SimulatorResult::Type::AlgorithmMisbehaved;
                        result->output_messages.emplace_back("Failing because: Command to set timer at: " + std::to_string(command.timer_should_fire_at()) + " which is in the past");
                        return *result;
                    }
                } else if (command.type() == AlgorithmResponse::Type::AlgorithmFailed || command.type() == AlgorithmResponse::Type::AlgorithmMisbehaved) {
                    result->type = command.type() == AlgorithmResponse::Type::AlgorithmFailed ? SimulatorResult::Type::AlgorithmFailed : SimulatorResult::Type::AlgorithmMisbehaved;
                    result->output_messages.insert(result->output_messages.end(), command.messages().begin(), command.messages().end());
                    return *result;
                } else {
                    ASSERT_NOT_REACHED();
                }
            }
        }

        next_request_time = m_generator->next_requests_at();
        if (next_request_time < last_requests) {
            result->type = SimulatorResult::Type::RequestGenerationFailed;
            return *result;
        }
    }

    return *result;
}

}

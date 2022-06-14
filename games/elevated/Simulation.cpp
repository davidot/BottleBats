#include "Simulation.h"
#include "../../util/Assertions.h"
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
    if (!m_generator) {
        m_result = {SimulatorResult::Type::GenerationFailed, {"No generator given"}};
        return false;
    }

    auto building_result = m_generator->generate_building();
    std::sort(building_result.blueprint().elevators.begin(), building_result.blueprint().elevators.end(), [](auto const& lhs, auto const& rhs) {
        return lhs.group < rhs.group;
    });

    if (building_result.has_error()) {
        m_result = {SimulatorResult::Type::GenerationFailed, building_result.errors()};
        return false;
    }

    auto accepted = m_algorithm->accept_scenario_description(building_result);

    if (accepted.type != ElevatedAlgorithm::ScenarioAccepted::Type::Accepted) {
        m_result = {
            accepted.type == ElevatedAlgorithm::ScenarioAccepted::Type::Rejected ?
                 SimulatorResult::Type::AlgorithmRejected : SimulatorResult::Type::AlgorithmFailed,
            std::move(accepted.messages)
        };
        return false;
    }

    m_event_distributor.on_initial_building(building_result.blueprint());

    m_building = BuildingState { building_result.extract_blueprint(), &m_event_distributor };

    return true;
}

constexpr Time extra_time_after_last_request = 100000;

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

Simulation::SimulationDone Simulation::tick()
{
    if (m_result.type == SimulatorResult::Type::Starting) {
        if (!m_algorithm) {
            m_result = {SimulatorResult::Type::AlgorithmFailed, {"No algorithm given"}};
            return SimulationDone::Yes;
        }
        if (!setup_for_run())
            return SimulationDone::Yes;
        
        m_result.type = SimulatorResult::Type::Running;
    }
    
    ASSERT(m_result.type == SimulatorResult::Type::Running);

#define STOP_SIMULATION(type, messages) \
    do { m_result = {type, messages}; return SimulationDone::Yes; } while(false)

    NextRequests next_request_time = m_generator->next_requests_at();
    if (next_request_time.type == NextRequests::Type::Done && m_building.passengers_done())
        STOP_SIMULATION(SimulatorResult::Type::SuccessFull, {});

    auto next_building_event = m_building.next_event_at();

    auto next_time_or_none = min_time(next_request_time, next_building_event, m_next_timer);
    if (!next_time_or_none.has_value())
        STOP_SIMULATION(SimulatorResult::Type::NoNextEvent, {});

    auto running_until = *next_time_or_none;

    if (next_request_time.type != NextRequests::Type::At && running_until > m_last_requests + extra_time_after_last_request)
        STOP_SIMULATION(SimulatorResult::Type::FailedToResolveAllRequests, {});

    auto elevator_updates = m_building.update_until(running_until);
    ASSERT(m_generator->next_requests_at() > running_until || next_request_time == running_until);

    std::vector<ElevatorID> elevators_closed;
    {
        for (auto& elevator_update : elevator_updates) {
            if (elevator_update.type == BuildingState::UpdateResult::Type::DoorsOpened) {
                auto callback = m_algorithm->on_doors_open(running_until, elevator_update.id, m_building);
                if (callback.has_value())
                    m_building.transfer_passengers(elevator_update.id, callback.value());
                else
                    m_building.transfer_passengers(elevator_update.id);
            } else {
                ASSERT(elevator_update.type == BuildingState::UpdateResult::Type::DoorsClosed);
                elevators_closed.push_back(elevator_update.id);
            }
        }
    }

    std::vector<AlgorithmInput> inputs;

    if (next_request_time == running_until) {
        m_last_requests = running_until;
        auto new_requests = m_generator->requests_at(running_until);
        inputs.reserve(new_requests.size() + elevator_updates.size());
        for (auto& new_request : new_requests) {
            auto queue_index = m_building.add_request(new_request);
            if (!queue_index.has_value()) {
                STOP_SIMULATION(SimulatorResult::Type::RequestGenerationFailed,
                    {"Failing because: Added request from: " + std::to_string(new_request.from) + " to " + std::to_string(new_request.to) + " within group " + std::to_string(new_request.group)});
            }
            inputs.push_back(AlgorithmInput::new_request(new_request.from, queue_index.value()));
        }
    }

    inputs.reserve(elevators_closed.size());
    std::transform(elevators_closed.begin(), elevators_closed.end(), std::back_inserter(inputs), [](ElevatorID id){
        return AlgorithmInput::elevator_closed_doors(id);
    });

    if (m_next_timer == running_until)
        inputs.push_back(AlgorithmInput::timer_fired());

    m_next_timer.reset();

    if (!inputs.empty()) {
        auto commands = m_algorithm->on_inputs(running_until, m_building, inputs);

        for (auto& command : commands) {
            if (command.type() == AlgorithmResponse::Type::MoveElevator) {
                if (!m_building.send_elevator(command.elevator_to_move(), command.elevator_target())) {
                    STOP_SIMULATION(SimulatorResult::Type::AlgorithmMisbehaved,
                        {"Failing because: Command to move elevator: " + std::to_string(command.elevator_to_move()) + " to " + std::to_string(command.elevator_target())});
                }
            } else if (command.type() == AlgorithmResponse::Type::SetTimer) {
                m_next_timer = command.timer_should_fire_at();
                if (m_next_timer <= running_until) {
                    STOP_SIMULATION(
                        SimulatorResult::Type::AlgorithmMisbehaved,
                        {"Failing because: Command to set timer at: " + std::to_string(command.timer_should_fire_at()) + " which is in the past"}
                    );
                }
            } else if (command.type() == AlgorithmResponse::Type::AlgorithmFailed || command.type() == AlgorithmResponse::Type::AlgorithmMisbehaved) {
                STOP_SIMULATION(
                    command.type() == AlgorithmResponse::Type::AlgorithmFailed ? SimulatorResult::Type::AlgorithmFailed : SimulatorResult::Type::AlgorithmMisbehaved,
                        command.messages()
                    );
            } else {
                ASSERT_NOT_REACHED();
            }
        }
    }

    next_request_time = m_generator->next_requests_at();
    if (next_request_time < m_last_requests)
        STOP_SIMULATION(SimulatorResult::Type::RequestGenerationFailed, {});

    return SimulationDone::No;
#undef STOP_SIMULATION
}

SimulatorResult Simulation::run_full_simulation()
{
    SimulationDone done;
    do {
        done = tick();
    } while(done == SimulationDone::No);
    ASSERT(!m_result.is_in_progress());
    return m_result;
}

SimulatorResult Simulation::result() const
{
    ASSERT(!m_result.is_in_progress());
    return m_result;
}

}

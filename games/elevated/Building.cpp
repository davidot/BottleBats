#include <set>
#include <algorithm>
#include "Building.h"
#include "../../util/Assertions.h"

namespace Elevated {

BuildingState::BuildingState(BuildingBlueprint blueprint)
{
    std::set<Height> floors;

    std::vector<Height> start_floors;
    start_floors.reserve(blueprint.reachable_per_group.size());
    for (auto& reachable_floors : blueprint.reachable_per_group) {
        ASSERT(!reachable_floors.empty());
        start_floors.emplace_back(*std::min_element(reachable_floors.begin(), reachable_floors.end()));
        floors.insert(reachable_floors.begin(), reachable_floors.end());
    }

    m_floors.reserve(floors.size());
    for (auto floor : floors)
        m_floors.insert({floor, {}});


    ElevatorID id {0};

    for (auto& elevator : blueprint.elevators) {
        ASSERT(elevator.group < blueprint.reachable_per_group.size());
        m_elevators.emplace_back(id++, elevator.group, start_floors[elevator.group]);
    }

    m_group_reachable = std::move(blueprint.reachable_per_group);
}

void BuildingState::update_until(Time target_time)
{
    ASSERT(target_time > m_current_time);
    ASSERT(target_time <= next_event_at().value_or(target_time));

    Time steps = target_time - m_current_time;
    m_current_time = target_time;

    std::vector<ElevatorID> elevators_closed_doors;

    for (auto& elevator : m_elevators) {
        auto result = elevator.update(steps);
        switch (result) {
        case ElevatorState::ElevatorUpdateResult::Nothing:
            m_distributor.on_elevator_stopped(m_current_time, steps, elevator);
            break;
        case ElevatorState::ElevatorUpdateResult::DoorsOpened: {
            ASSERT(m_floors.contains(elevator.height()));
            m_distributor.on_elevator_opened_doors(m_current_time, elevator);

            auto& floor_stopped_at = m_floors[elevator.height()];
            auto transferred = elevator.transfer_passengers(floor_stopped_at);
            for (auto& arrived_passenger_id : transferred.dropped_off_passengers)
                m_distributor.on_passenger_leave_elevator(m_current_time, arrived_passenger_id, elevator.height());

            for (auto& picked_up_passenger : transferred.picked_up_passengers)
                m_distributor.on_passenger_enter_elevator(m_current_time, picked_up_passenger, elevator.id);

            break;
        }
        case ElevatorState::ElevatorUpdateResult::DoorsClosed:
            ASSERT(m_floors.contains(elevator.height()));
            m_distributor.on_elevator_closed_doors(m_current_time, elevator);
            elevators_closed_doors.push_back(elevator.id);
            break;
        }
    }
}


void BuildingState::add_request(Passenger passenger)
{
    ASSERT(m_floors.contains(passenger.from));
    ASSERT(m_floors.contains(passenger.to));
    ASSERT(passenger.group < m_group_reachable.size());
    ASSERT(m_group_reachable[passenger.group].contains(passenger.from));

    if (passenger.group >= m_group_reachable.size() || !m_group_reachable[passenger.group].contains(passenger.from))
        return; // FIXME: We might want to warn or fail here?

    m_distributor.on_request_created(m_current_time, passenger);
    m_floors[passenger.from].emplace_back(passenger);
}

void BuildingState::send_elevator(ElevatorID id, Height target)
{
    ASSERT(id < m_elevators.size());
    auto& elevator = m_elevators[id];
    m_distributor.on_elevator_set_target(m_current_time, target, elevator);
    elevator.set_target(target);
}

std::optional<Time> BuildingState::next_event_at() const
{
    std::optional<Time> first_time;
    for (auto& elevator : m_elevators) {
        auto time_or_none = elevator.time_until_next_event();
        if (!time_or_none.has_value())
            continue;

        Time next_value = m_current_time + time_or_none.value();

        if (!first_time.has_value() || first_time.value() > next_value)
            first_time = next_value;
    }

    return first_time;
}


}

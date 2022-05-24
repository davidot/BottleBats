#include <set>
#include <algorithm>
#include "Building.h"
#include "../../util/Assertions.h"

namespace Elevated {

BuildingState::BuildingState(BuildingBlueprint blueprint, EventListener* event_listener)
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
        m_elevators.emplace_back(id++, elevator, start_floors[elevator.group]);
    }

    m_group_reachable = std::move(blueprint.reachable_per_group);
    m_event_listener = event_listener;
    ASSERT(m_event_listener);
}

std::vector<BuildingState::UpdateResult> BuildingState::update_until(Time target_time)
{
    ASSERT(m_event_listener);
    ASSERT(target_time >= m_current_time);
    ASSERT(target_time <= next_event_at().value_or(target_time));

    Time steps = target_time - m_current_time;

    if (steps == 0)
        return {};

    m_current_time = target_time;

    std::vector<UpdateResult> elevators_closed_doors;

    for (auto& elevator : m_elevators) {
        ASSERT(elevator.current_state() != ElevatorState::State::DoorsOpen);
        Height initial_height = elevator.height();
        auto result = elevator.update(steps);
        switch (result) {
        case ElevatorState::ElevatorUpdateResult::Nothing:
            if (auto new_height = elevator.height(); new_height != initial_height)
                m_event_listener->on_elevator_moved(m_current_time, distance_between(initial_height, new_height), initial_height, elevator);
            else
                m_event_listener->on_elevator_stopped(m_current_time, steps, elevator);
            break;
        case ElevatorState::ElevatorUpdateResult::DoorsOpened: {
            ASSERT(m_floors.contains(elevator.height()));
            if (auto new_height = elevator.height(); new_height != initial_height)
                m_event_listener->on_elevator_moved(m_current_time, distance_between(initial_height, new_height), initial_height, elevator);

            m_event_listener->on_elevator_opened_doors(m_current_time, elevator);
            elevators_closed_doors.push_back({UpdateResult::Type::DoorsOpened, elevator.id});
            break;
        }
        case ElevatorState::ElevatorUpdateResult::DoorsClosed:
            ASSERT(initial_height == elevator.height());
            ASSERT(m_floors.contains(elevator.height()));
            m_event_listener->on_elevator_closed_doors(m_current_time, elevator);
            elevators_closed_doors.push_back({UpdateResult::Type::DoorsClosed, elevator.id});
            break;
        }
    }

    return elevators_closed_doors;
}

void BuildingState::transfer_passengers(ElevatorID id, ElevatorState::PassengerCallback const& callback)
{
    ASSERT(id < m_elevators.size());
    auto& elevator = m_elevators[id];
    ASSERT(elevator.current_state() == ElevatorState::State::DoorsOpen);
    if (elevator.current_state() != ElevatorState::State::DoorsOpen)
        return;

    auto& floor_stopped_at = m_floors[elevator.height()];
    auto transferred = elevator.transfer_passengers(floor_stopped_at, callback);
    for (auto& arrived_passenger_id : transferred.dropped_off_passengers)
        m_event_listener->on_passenger_leave_elevator(m_current_time, arrived_passenger_id, elevator.height());

    for (auto& picked_up_passenger : transferred.picked_up_passengers)
        m_event_listener->on_passenger_enter_elevator(m_current_time, picked_up_passenger, elevator.id);
}

std::optional<size_t> BuildingState::add_request(PassengerBlueprint passenger)
{
    ASSERT(m_event_listener);
    ASSERT(m_floors.contains(passenger.from));
    ASSERT(m_floors.contains(passenger.to));
    ASSERT(passenger.group < m_group_reachable.size());
    ASSERT(m_group_reachable[passenger.group].contains(passenger.from));

    if (passenger.group >= m_group_reachable.size() || !m_group_reachable[passenger.group].contains(passenger.from))
        return {};

    ASSERT(m_next_passenger_id != 0);
    auto& new_passenger = m_floors[passenger.from].emplace_back(m_next_passenger_id++, passenger);
    m_event_listener->on_request_created(m_current_time + 1, new_passenger);
    return m_floors[passenger.from].size() - 1u;
}

bool BuildingState::send_elevator(ElevatorID id, Height target)
{
    ASSERT(m_event_listener);
    ASSERT(id < m_elevators.size());

    if (id >= m_elevators.size())
        return false;

    auto& elevator = m_elevators[id];

    if (!m_group_reachable[elevator.group_id].contains(target))
        return false;

    m_event_listener->on_elevator_set_target(m_current_time, target, elevator);
    elevator.set_target(target);

    return true;
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

std::vector<Passenger> const &BuildingState::passengers_at(Height height) const {
    ASSERT(m_floors.contains(height));
    return m_floors.find(height)->second;
}

ElevatorState const &BuildingState::elevator(ElevatorID id) const {
    ASSERT(id < m_elevators.size());
    return m_elevators[id];
}

bool BuildingState::passengers_done()
{
    return std::all_of(m_floors.begin(), m_floors.end(), [&](const auto& item){
        return item.second.empty();
    }) && std::all_of(m_elevators.begin(), m_elevators.end(), [&](ElevatorState const& elevator_state) {
        return elevator_state.passengers().empty();
    });
}

}

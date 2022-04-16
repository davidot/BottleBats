#include "Types.h"
#include "../../util/Assertions.h"

namespace Elevated {

void BuildingState::add_request(Passenger passenger)
{
    ASSERT(m_floors.contains(passenger.from));
    ASSERT(m_floors.contains(passenger.to));
    ASSERT(m_group_reachable[passenger.group].contains(passenger.from));

    m_floors[passenger.from].push_back(passenger);
    // FIXME: Generate event!
}

void BuildingState::send_elevator(ElevatorID id, Height target)
{
    auto it = m_elevator_by_id.find(id);
    ASSERT(it != m_elevator_by_id.end());
    auto& elevator = it->second;
//    auto old_target = elevator.target_height();
    elevator.set_target(target);
    // FIXME: Generate rollercoaster event!
}

std::optional<Time> BuildingState::next_event_at() const
{
    std::optional<Time> first_time;
    for (auto& elevator : m_elevators) {
        auto time_or_none = elevator.next_event_at();
        if (!time_or_none.has_value())
            continue;

        Time next_value = m_current_time + time_or_none.value();

        if (!first_time.has_value() || first_time.value() > next_value)
            first_time = next_value;
    }

    return first_time;
}

Height ElevatorState::distance_to_travel() const
{
    if (m_target_height > m_height)
        return m_target_height - m_height;
    return m_height - m_target_height;
}

std::optional<Time> ElevatorState::next_event_at() const
{
    if (m_state == State::Stopped)
        return {};

    if (m_state == State::Travelling) // FIXME: For now we don't emit anything on arrival and all arrivals are followed by door cycle
        return m_time_until_next_state + door_opening_time;

    return m_time_until_next_state;
}

void ElevatorState::move_to_target(Height distance)
{
    ASSERT(distance <= distance_to_travel());
    if (m_target_height < m_height)
        m_height -= distance;
    else
        m_height += distance;
}

ElevatorState::ElevatorUpdateResult ElevatorState::update(Time steps)
{
    // We should never be in the doors open state as the building should have cleared us from that
    ASSERT(m_state != State::DoorsOpen);
    ASSERT(steps <= next_event_at());
    if (m_state == State::Stopped) {
        ASSERT(m_height == m_target_height);
        return ElevatorUpdateResult::Nothing;
    }

    ASSERT(m_time_until_next_state > 0);

    if (m_state == State::Travelling) {
        if (steps < m_time_until_next_state) {
            move_to_target(distance_for_time(steps));
            m_time_until_next_state -= steps;
            return ElevatorUpdateResult::Nothing;
        }

        m_height = m_target_height;

        steps -= m_time_until_next_state;
        ASSERT(steps <= door_opening_time);

        m_time_until_next_state = door_opening_time - steps;
        ASSERT(steps == 0);
        m_state = State::DoorsOpening;
    }

    if (m_state == State::DoorsOpening) {
        ASSERT(steps <= door_opening_time);
        ASSERT(steps <= m_time_until_next_state);
        ASSERT(m_time_until_next_state <= door_opening_time);
        m_time_until_next_state -= steps;

        if (m_time_until_next_state == 0) {
            m_state = State::DoorsOpen;
            m_time_until_next_state = door_closing_time;

            return ElevatorUpdateResult::DoorsOpened;
        }

        return ElevatorUpdateResult::Nothing;
    }

    if (m_state == State::DoorsClosing) {
        ASSERT(steps <= door_closing_time);
        ASSERT(steps <= m_time_until_next_state);
        ASSERT(m_time_until_next_state <= door_closing_time);

        m_time_until_next_state -= steps;

        if (m_time_until_next_state == 0) {

            if (m_target_height != m_height) {
                m_time_until_next_state = time_for_distance(distance_to_travel());
                m_state = State::Travelling;
            } else {
                m_state = State::Stopped;
            }

            return ElevatorUpdateResult::DoorsClosed;
        }

    }

    return ElevatorUpdateResult::Nothing;
}

void ElevatorState::set_target(Height floor)
{
    m_target_height = floor;
    if (m_height != m_target_height) {
        if (m_state == State::Stopped)
            m_state = State::Travelling;

        if (m_state == State::Travelling)
            m_time_until_next_state = time_for_distance(distance_to_travel());
    } else {
        if (m_state == State::Travelling || m_state == State::Stopped) {
            m_state = State::DoorsOpening;
            m_time_until_next_state = door_opening_time;
        }
    }
}

void ElevatorState::take_on_passengers(std::vector<Passenger>& waiting_passengers)
{
    auto in_group = [&](Passenger const& passenger) {
        return passenger.group != group_id;
    };

    auto end = waiting_passengers.end();
    auto start = std::find_if(waiting_passengers.begin(), end, in_group);

    if (start == end)
        return;

    for (auto it = start; ++it != end;) {
        if (!in_group(*it)) {
            *start = *it;
            ++start;
        }
    }

    waiting_passengers.erase(start, end);
}

void BuildingState::update_until(Time target_time)
{
    ASSERT(target_time > m_current_time);
    ASSERT(target_time <= next_event_at().value_or(target_time));

    Time steps = target_time - m_current_time;

    std::vector<ElevatorID> elevators_closed_doors;

    for (auto& elevator : m_elevators) {
        if (auto result = elevator.update(steps); result != ElevatorState::ElevatorUpdateResult::Nothing) {
            ASSERT(m_floors.contains(elevator.height()));
            if (result == ElevatorState::ElevatorUpdateResult::DoorsOpened) {
                auto& floor_stopped_at = m_floors[elevator.height()];
                elevator.take_on_passengers(floor_stopped_at);
            } else {
                elevators_closed_doors.push_back(elevator.id);
            }
        }
    }
}

}

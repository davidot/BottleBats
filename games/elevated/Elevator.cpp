#include "Elevator.h"
#include "../../util/Assertions.h"

namespace Elevated {

std::optional<Time> ElevatorState::time_until_next_event() const
{
    if (m_state == State::Stopped || m_state == State::DoorsOpen)
        return {};

    if (m_state == State::Travelling) // FIXME: For now we don't emit anything on arrival and all arrivals are followed by door cycle
        return m_time_until_next_state + door_opening_time;

    return m_time_until_next_state;
}

void ElevatorState::move_to_target(Height distance)
{
    ASSERT(distance <= distance_between(m_height, m_target_height));
    if (m_target_height < m_height)
        m_height -= distance;
    else
        m_height += distance;
}

ElevatorState::ElevatorUpdateResult ElevatorState::update(Time steps)
{
    // We should never be in the doors open state as the building should have cleared us from that
    ASSERT(m_state != State::DoorsOpen);
    ASSERT(steps <= time_until_next_event());
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
        steps = 0;
        m_state = State::DoorsOpening;
    }

    if (m_state == State::DoorsOpening) {
        ASSERT(steps <= door_opening_time);
        ASSERT(steps <= m_time_until_next_state);
        ASSERT(m_time_until_next_state <= door_opening_time);
        m_time_until_next_state -= steps;

        if (m_time_until_next_state == 0) {
            m_state = State::DoorsOpen;

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
                m_time_until_next_state = time_for_distance(distance_between(m_height, m_target_height));
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
            m_time_until_next_state = time_for_distance(distance_between(m_height, m_target_height));
    } else {
        if (m_state == State::Travelling || m_state == State::Stopped) {
            m_state = State::DoorsOpening;
            m_time_until_next_state = door_opening_time;
        }
    }
}

void ElevatorState::dropoff_passengers(TransferredPassengers& transferred)
{
    auto not_at_arrival = [height= m_height](TravellingPassenger const& passenger) {
        return passenger.to != height;
    };

    auto reached_destination = std::partition(m_passengers.begin(), m_passengers.end(), not_at_arrival);

    transferred.dropped_off_passengers.reserve(std::distance(reached_destination, m_passengers.end()));
    std::transform(reached_destination, m_passengers.end(), std::back_inserter(transferred.dropped_off_passengers), [](TravellingPassenger const& passenger) {
        return passenger.id;
    });

    m_passengers.erase(reached_destination, m_passengers.end());
}

void ElevatorState::pickup_passengers(std::vector<Passenger>& waiting_passengers, TransferredPassengers& transferred)
{
    auto in_group = [&](Passenger const& passenger) {
        return passenger.group == group_id;
    };

    auto end = waiting_passengers.end();
    auto start = std::find_if(waiting_passengers.begin(), end, in_group);

    if (start == end)
        return;

    for (auto it = start; it != end; ++it) {
        if (in_group(*it)) {
            transferred.picked_up_passengers.emplace_back(*it);
            m_passengers.push_back({it->id, it->to});
        } else {
            *start = *it;
            ++start;
        }
    }

    waiting_passengers.erase(start, end);
}

ElevatorState::TransferredPassengers ElevatorState::transfer_passengers(std::vector<Passenger>& waiting_passengers)
{
    ASSERT(m_state == State::DoorsOpen);
    m_state = State::DoorsClosing;
    m_time_until_next_state = door_closing_time;

    TransferredPassengers transferred;

    dropoff_passengers(transferred);
    pickup_passengers(waiting_passengers, transferred);

    return transferred;
}


}

#include "PassengerStats.h"
#include "../../../util/Assertions.h"
#include <numeric>

namespace Elevated {

void PassengerStatsListener::on_request_created(Time at, Passenger const& passenger)
{
    ASSERT(!m_arrival_times.contains(passenger.id));
    ASSERT(!m_enter_times.contains(passenger.id));
    m_arrival_times[passenger.id] = at;
}

void PassengerStatsListener::on_passenger_enter_elevator(Time at, Passenger const& passenger, ElevatorID)
{
    ASSERT(m_arrival_times.contains(passenger.id));
    ASSERT(!m_enter_times.contains(passenger.id));
    ASSERT(at >= m_arrival_times[passenger.id]);

    Time wait_time = at - m_arrival_times[passenger.id];
    m_wait_times.add_observation(wait_time);
    m_enter_times[passenger.id] = at;
    m_arrival_times.erase(passenger.id);
}

void PassengerStatsListener::on_passenger_leave_elevator(Time at, PassengerID id, Height)
{
    ASSERT(!m_arrival_times.contains(id));
    ASSERT(m_enter_times.contains(id));

    Time travel_time = at - m_enter_times[id];
    m_travel_times.add_observation(travel_time);
    m_enter_times.erase(id);

    if (auto times_opened = m_door_opened_counts.find(id); times_opened != m_door_opened_counts.end()) {
        m_times_door_opened.add_observation(times_opened->second);
        m_door_opened_counts.erase(times_opened);
    } else {
        m_times_door_opened.add_observation(0);
    }
}

void PassengerStatsListener::on_elevator_opened_doors(Time, ElevatorState const& state)
{
    for (auto& passenger : state.passengers()) {
        if (passenger.to == state.height())
            continue;
        ++m_door_opened_counts[passenger.id];
    }}

}

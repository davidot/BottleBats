#include "PassengerStats.h"
#include "../../../util/Assertions.h"

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
    ++m_wait_times[wait_time];
    m_max_wait_time = std::max(m_max_wait_time, wait_time);
    m_enter_times[passenger.id] = at;
    m_arrival_times.erase(passenger.id);
}

void PassengerStatsListener::on_passenger_leave_elevator(Time at, PassengerID id, Height)
{
    ASSERT(!m_arrival_times.contains(id));
    ASSERT(m_enter_times.contains(id));

    Time travel_time = at - m_enter_times[id];
    ++m_travel_times[travel_time];
    m_max_travel_time = std::max(m_max_travel_time, travel_time);
    m_enter_times.erase(id);
}

}

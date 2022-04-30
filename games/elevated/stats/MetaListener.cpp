#include "MetaListener.h"

namespace Elevated {

void MetaListener::on_elevator_opened_doors(Time time, ElevatorState const&)
{
    ++m_events;
    times.insert(time);
}

void MetaListener::on_elevator_closed_doors(Time time, ElevatorState const&)
{
    ++m_events;
    times.insert(time);
}

void MetaListener::on_elevator_stopped(Time at, Time, ElevatorState const&)
{
    ++m_events;
    times.insert(at);
}

void MetaListener::on_elevator_moved(Time time, Height, ElevatorState const&)
{
    ++m_events;
    times.insert(time);
}

void MetaListener::on_request_created(Time, Passenger const&)
{
    ++m_events;
}

void MetaListener::on_passenger_enter_elevator(Time, Passenger const&, ElevatorID)
{
    ++m_events;
}

void MetaListener::on_passenger_leave_elevator(Time, PassengerID, Height)
{
    ++m_events;
}

void MetaListener::on_elevator_set_target(Time, Height, ElevatorState const&)
{
    ++m_events;
}

}

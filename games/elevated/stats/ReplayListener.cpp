#include "ReplayListener.h"

namespace Elevated {

void ReplayListener::on_elevator_opened_doors(Time time, const ElevatorState& state)
{
    current_stream << time << " opened " << state.id << ' ' << state.height() << '\n';
}

void ReplayListener::on_elevator_closed_doors(Time time, const ElevatorState& state)
{
    current_stream << time << " closed " << state.id << ' ' << state.height() << '\n';
}

void ReplayListener::on_elevator_stopped(Time at, Time duration, const ElevatorState& state)
{
    current_stream << at << " stopped " << state.id << ' ' << duration << '\n';
}

void ReplayListener::on_elevator_moved(Time time, Height height, Height initial_height, const ElevatorState& state)
{
    current_stream << time << " moved " << state.id << ' ' << height << ' ' << initial_height << '\n';
}

void ReplayListener::on_request_created(Time time, const Passenger& passenger)
{
    current_stream << time << " request "
                   << passenger.from << ' '
                   << passenger.to << ' '
                   << passenger.group << ' '
                   << passenger.capacity << ' '
                   << passenger.id << '\n';
}

void ReplayListener::on_passenger_enter_elevator(Time time, const Passenger& passenger, ElevatorID id)
{
    current_stream << time << " enter " << passenger.id << ' ' << id << '\n';
}

void ReplayListener::on_passenger_leave_elevator(Time time, PassengerID id, Height)
{
    current_stream << time << " leave " << id << '\n';
}

void ReplayListener::on_elevator_set_target(Time time, Height new_target, const ElevatorState& state)
{
    current_stream << time << " target " << state.id << ' ' << new_target << ' ' << state.height() << '\n';
}

}

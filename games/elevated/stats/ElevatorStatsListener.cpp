#include "ElevatorStatsListener.h"

namespace Elevated {

void ElevatorStatsListener::on_elevator_opened_doors(Time time, const ElevatorState& state)
{
    m_times_doors_opened.add_to_observation(state.id, 1);
}

void ElevatorStatsListener::on_elevator_moved(Time time, Height distance_travelled, Height before_height, const ElevatorState& state)
{
    m_distance_travelled.add_to_observation(state.id, distance_travelled);
}

void ElevatorStatsListener::on_elevator_stopped(Time at, Time duration, const ElevatorState& state)
{
    m_time_stopped.add_to_observation(state.id, duration);
}

}

#include "ElevatorStatsListener.h"

namespace Elevated {

void ElevatorStatsListener::on_elevator_opened_doors(Time, const ElevatorState& state)
{
    m_times_doors_opened.add_to_observation(state.id, 1);
}

void ElevatorStatsListener::on_elevator_moved(Time, Height distance_travelled, Height, const ElevatorState& state)
{
    m_distance_travelled.add_to_observation(state.id, distance_travelled);
}

void ElevatorStatsListener::on_elevator_stopped(Time, Time duration, const ElevatorState& state)
{
    m_time_stopped.add_to_observation(state.id, duration);
}

}

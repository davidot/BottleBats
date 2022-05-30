#include "PowerStatsListener.h"
#include <numeric>

namespace Elevated {

void PowerStatsListener::on_elevator_opened_doors(Time, ElevatorState const&)
{
    ++m_times_door_opened;
}

void PowerStatsListener::on_elevator_moved(Time, Height height_travelled, Height, ElevatorState const& state)
{
    m_distance_moved[state.filled_capacity()] += height_travelled;
}

size_t PowerStatsListener::total_distance_travelled() const
{
    return std::accumulate(m_distance_moved.begin(), m_distance_moved.end(), size_t(0), [](uint64_t acc, auto& entry) {
        return acc + entry.second;
    });
}

void PowerStatsListener::on_elevator_stopped(Time, Time duration, ElevatorState const& state)
{
    if (!state.passengers().empty())
        m_time_stopped_with_passengers += duration;
}

}

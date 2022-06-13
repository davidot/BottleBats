#include "SpecialEventsListener.h"
#include "../../../util/Assertions.h"
#include <numeric>

namespace Elevated {

void SpecialEventsListener::on_elevator_set_target(Time, Height new_target, ElevatorState const& state)
{
    if (state.current_state() != ElevatorState::State::Travelling)
        return;

    Height old_target = state.target_height();

    if (new_target == old_target)
        return;

    Height current = state.height();

    if (new_target == current)
        ++m_travelling_stop[state.filled_capacity()];
    else if ((new_target > current) != (old_target > current))
        ++m_roller_coaster_events[state.filled_capacity()];
}

uint64_t SpecialEventsListener::total_roller_coaster_events() const
{
    return std::accumulate(m_roller_coaster_events.begin(), m_roller_coaster_events.end(), uint64_t(0), [](uint64_t acc, auto& entry) {
        return acc + entry.second;
    });
}

uint64_t SpecialEventsListener::total_travelling_stops() const
{
    return std::accumulate(m_travelling_stop.begin(), m_travelling_stop.end(), uint64_t(0), [](uint64_t acc, auto& entry) {
        return acc + entry.second;
    });
}

}

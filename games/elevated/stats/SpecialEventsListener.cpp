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

size_t SpecialEventsListener::total_roller_coaster_events() const
{
    return std::accumulate(m_roller_coaster_events.begin(), m_roller_coaster_events.end(), 0ul, [](uint64_t acc, auto& entry) {
        return acc + entry.second;
    });
}

}

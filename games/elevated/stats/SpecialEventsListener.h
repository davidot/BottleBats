#pragma once

#include "Listener.h"

namespace Elevated {

class SpecialEventsListener final : public EventListener {
public:
    virtual void on_elevator_set_target(Time time, Height new_target, ElevatorState const& state) override;

    uint64_t total_roller_coaster_events() const;
    uint64_t total_travelling_stops() const;

private:
    std::unordered_map<Capacity, uint64_t> m_roller_coaster_events;
    std::unordered_map<Capacity, uint64_t> m_travelling_stop;
};

}

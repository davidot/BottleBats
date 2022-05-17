#pragma once

#include "Listener.h"

namespace Elevated {

class PowerStatsListener final : public EventListener {
public:
    virtual void on_elevator_opened_doors(Time time, ElevatorState const& state) override;
    virtual void on_elevator_moved(Time time, Height height, Height before_height, ElevatorState const& state) override;
    virtual void on_elevator_stopped(Time at, Time duration, ElevatorState const& state) override;

    size_t times_door_opened() const { return m_times_door_opened; }
    size_t total_distance_travelled() const;
    Time time_stopped_with_passengers() const { return m_time_stopped_with_passengers; }

private:
    std::unordered_map<Capacity, uint64_t> m_distance_moved;
    uint64_t m_times_door_opened{0};
    Time m_time_stopped_with_passengers{0};
};

}

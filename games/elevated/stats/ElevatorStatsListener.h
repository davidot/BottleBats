#pragma once

#include "../../../util/Histogram.h"
#include "Listener.h"
namespace Elevated {

class ElevatorStatsListener : public EventListener {
public:
    virtual void on_elevator_opened_doors(Time time, const ElevatorState& state) override;
    virtual void on_elevator_moved(Time time, Height distance_travelled, Height before_height, const ElevatorState& state) override;
    virtual void on_elevator_stopped(Time at, Time duration, const ElevatorState& state) override;

    [[nodiscard]] Height max_travel_distance() const { return m_distance_travelled.max_value(); }
    [[nodiscard]] Height min_travel_distance() const { return m_distance_travelled.min_value(); }
    [[nodiscard]] double avg_travel_distance() const { return m_distance_travelled.avg_value();}
    [[nodiscard]] Height total_travel_distance() const { return m_distance_travelled.sum_value(); }

    [[nodiscard]] size_t max_doors_opened() const { return m_times_doors_opened.max_value(); }
    [[nodiscard]] size_t min_doors_opened() const { return m_times_doors_opened.min_value(); }
    [[nodiscard]] double avg_doors_opened() const { return m_times_doors_opened.avg_value(); }

    [[nodiscard]] size_t max_time_stopped() const { return m_time_stopped.max_value(); }
    [[nodiscard]] size_t min_time_stopped() const { return m_time_stopped.min_value(); }
    [[nodiscard]] double avg_time_stopped() const { return m_time_stopped.avg_value(); }

private:
    util::StoringHistogram<ElevatorID, Height> m_distance_travelled;
    util::StoringHistogram<ElevatorID, size_t> m_times_doors_opened;
    util::StoringHistogram<ElevatorID, Time> m_time_stopped;
};

}

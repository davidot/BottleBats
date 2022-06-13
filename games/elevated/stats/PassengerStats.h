#pragma once

#include "../../../util/Histogram.h"
#include "Listener.h"

namespace Elevated {

class PassengerStatsListener final : public EventListener {
public:
    virtual void on_request_created(Time at, Passenger const& passenger) override;
    virtual void on_passenger_enter_elevator(Time at, Passenger const& passenger, ElevatorID id) override;
    virtual void on_passenger_leave_elevator(Time at, PassengerID id, Height height) override;
    virtual void on_elevator_opened_doors(Time time, ElevatorState const& state) override;

    Time max_wait_times() const { return m_wait_times.max_value(); }
    Time max_travel_times() const { return m_travel_times.max_value(); }
    uint32_t max_times_door_opened() const { return m_times_door_opened.max_value(); }
    uint32_t first_stop_passengers() const { return m_wait_times.total_entries() - m_times_door_opened.total_entries(); }

    [[nodiscard]] double average_wait_time() const { return (m_wait_times.sum_of_values<double>()) / (double)m_wait_times.total_entries(); }
    [[nodiscard]] double average_travel_time() const { return (m_travel_times.sum_of_values<double>()) / (double)m_travel_times.total_entries(); }
private:
    std::unordered_map<PassengerID, Time> m_arrival_times;
    std::unordered_map<PassengerID, Time> m_enter_times;

    util::Histogram<Time> m_wait_times;
    util::Histogram<Time> m_travel_times;
    util::Histogram<uint32_t> m_times_door_opened;

    std::unordered_map<PassengerID, uint32_t> m_door_opened_counts;
};


}

#pragma once

#include "Listener.h"
namespace Elevated {

class PassengerStatsListener final : public EventListener {
public:
    virtual void on_request_created(Time at, Passenger const& passenger) override;
    virtual void on_passenger_enter_elevator(Time at, Passenger const& passenger, ElevatorID id) override;
    virtual void on_passenger_leave_elevator(Time at, PassengerID id, Height height) override;
    virtual void on_elevator_opened_doors(Time time, ElevatorState const& state) override;

    Time max_wait_times() const { return m_max_wait_time; }
    Time max_travel_times() const { return m_max_travel_time; }
    uint32_t max_times_door_opened() const { return m_max_times_door_opened; }

    [[nodiscard]] double average_wait_time() const;
    [[nodiscard]] double average_travel_time() const;
private:
    std::unordered_map<PassengerID, Time> m_arrival_times;
    std::unordered_map<PassengerID, Time> m_enter_times;

    std::unordered_map<Time, uint64_t> m_wait_times;
    std::unordered_map<Time, uint64_t> m_travel_times;
    Time m_max_wait_time{0};
    Time m_max_travel_time{0};

    std::unordered_map<PassengerID, uint32_t> m_door_opened_counts;
    std::unordered_map<uint32_t, uint32_t> m_times_door_opened;
    uint32_t m_max_times_door_opened{0};
};


}

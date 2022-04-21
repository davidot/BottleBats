#pragma once

#include "Listener.h"
namespace Elevated {

class PassengerStatsListener : public EventListener {
public:
    void on_request_created(Time at, Passenger const& passenger) override;
    void on_passenger_enter_elevator(Time at, Passenger const& passenger, ElevatorID id) override;
    void on_passenger_leave_elevator(Time at, PassengerID id, Height height) override;

private:
    std::unordered_map<PassengerID, Time> m_arrival_times;
    std::unordered_map<PassengerID, Time> m_enter_times;

    std::unordered_map<Time, uint64_t> m_wait_times;
    std::unordered_map<Time, uint64_t> m_travel_times;
};


}

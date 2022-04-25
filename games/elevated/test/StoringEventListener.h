#pragma once

#include <elevated/stats/Listener.h>

namespace Elevated {

class StoringEventListener : public EventListener {
public:
    std::vector<std::pair<Time, Passenger>> request_created_events;
    void on_request_created(Time at, Passenger const& passenger) override
    {
        request_created_events.emplace_back(at, passenger);
    }

    std::vector<std::tuple<Time, Passenger, ElevatorID>> passenger_enter_events;
    void on_passenger_enter_elevator(Time at, Passenger const& passenger, ElevatorID id) override
    {
        passenger_enter_events.emplace_back(at, passenger, id);
    }

    std::vector<std::tuple<Time, PassengerID, Height>> passenger_leave_events;
    void on_passenger_leave_elevator(Time at, PassengerID id, Height height) override
    {
        passenger_leave_events.emplace_back(at, id, height);
    }

    std::vector<std::pair<Time, ElevatorState>> elevator_opened_events;
    void on_elevator_opened_doors(Time at, ElevatorState const& elevator) override
    {
        elevator_opened_events.emplace_back(at, elevator);
    }

    std::vector<std::pair<Time, ElevatorState>> elevator_closed_events;
    void on_elevator_closed_doors(Time at, ElevatorState const& elevator) override
    {
        elevator_closed_events.emplace_back(at, elevator);
    }

    std::vector<std::tuple<Time, Height, ElevatorState>> elevator_set_target_events;
    void on_elevator_set_target(Time at, Height new_target, ElevatorState const& elevator) override
    {
        elevator_set_target_events.emplace_back(at, new_target, elevator);
    }

    std::vector<std::tuple<Time, Time, ElevatorState>> elevator_stopped_events;
    void on_elevator_stopped(Time at, Time duration, ElevatorState const& elevator) override
    {
        elevator_stopped_events.emplace_back(at, duration, elevator);
    }

    [[nodiscard]] bool no_events() const
    {
        bool empty = request_created_events.empty()
            && passenger_enter_events.empty()
            && passenger_leave_events.empty()
            && elevator_opened_events.empty()
            && elevator_closed_events.empty()
            && elevator_set_target_events.empty()
            && elevator_stopped_events.empty();

        if (!empty)
            UNSCOPED_INFO(
                "request: " << request_created_events.size()
                            << ", enter: " << passenger_enter_events.size()
                            << ", leave: " << passenger_leave_events.size()
                            << ", opened: " << elevator_opened_events.size()
                            << ", closed: " << elevator_closed_events.size()
                            << ", target: " << elevator_set_target_events.size()
                            << ", stopped: " << elevator_stopped_events.size());
        return empty;
    }

    void clear_events()
    {
        request_created_events.clear();
        passenger_enter_events.clear();
        passenger_leave_events.clear();
        elevator_opened_events.clear();
        elevator_closed_events.clear();
        elevator_set_target_events.clear();
        elevator_stopped_events.clear();
    }
};

}

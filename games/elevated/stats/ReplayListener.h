#pragma once

#include "Listener.h"
#include <sstream>

namespace Elevated {

class ReplayListener final : public EventListener {
public:
    virtual void on_elevator_opened_doors(Time time, ElevatorState const& state) override;
    virtual void on_elevator_closed_doors(Time time, ElevatorState const& state) override;
    virtual void on_elevator_stopped(Time at, Time duration, ElevatorState const& state) override;
    virtual void on_elevator_moved(Time time, Height height, Height initial_height, ElevatorState const& state) override;
    virtual void on_request_created(Time time, Passenger const& passenger) override;
    virtual void on_passenger_enter_elevator(Time time, Passenger const& passenger, ElevatorID id) override;
    virtual void on_passenger_leave_elevator(Time time, PassengerID id, Height height) override;
    virtual void on_elevator_set_target(Time time, Height new_target, ElevatorState const& state) override;

    [[nodiscard]] std::string value() const { return m_current_stream.str(); }

private:
    std::ostringstream m_current_stream;
};

}

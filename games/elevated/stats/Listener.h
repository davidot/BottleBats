#pragma once
#include "../Elevator.h"
#include "../Types.h"
#include "../generation/Generation.h"
#include <memory>
#include <vector>

namespace Elevated {

class EventListener {
public:
    virtual ~EventListener() = default;

    virtual void on_initial_building(BuildingBlueprint const&) { }

    virtual void on_request_created(Time, Passenger const&) { }
    virtual void on_passenger_enter_elevator(Time, Passenger const&, ElevatorID) { }
    virtual void on_passenger_leave_elevator(Time, PassengerID, Height) { }

    virtual void on_elevator_opened_doors(Time, ElevatorState const&) { }
    virtual void on_elevator_closed_doors(Time, ElevatorState const&) { }

    virtual void on_elevator_set_target(Time, [[maybe_unused]] Height new_target, ElevatorState const&) { }
    virtual void on_elevator_stopped([[maybe_unused]] Time at, [[maybe_unused]] Time duration, ElevatorState const&) { }
    virtual void on_elevator_moved(Time, Height, ElevatorState const&) { }
};

class EventDistributor final : public EventListener {
public:
    void add_listener(std::shared_ptr<EventListener> listener);
    bool remove_listener(EventListener* listener);

    virtual void on_request_created(Time at, const Passenger& passenger) override;

    virtual void on_passenger_enter_elevator(Time at, const Passenger& passenger, ElevatorID id) override;

    virtual void on_passenger_leave_elevator(Time at, PassengerID id, Height height) override;

    virtual void on_elevator_opened_doors(Time at, const ElevatorState& elevator) override;

    virtual void on_elevator_closed_doors(Time at, const ElevatorState& elevator) override;

    virtual void on_elevator_set_target(Time at, Height new_target, const ElevatorState& elevator) override;

    virtual void on_elevator_stopped(Time at, Time duration, const ElevatorState& elevator) override;

    virtual void on_elevator_moved(Time at, Height distance, const ElevatorState& elevator) override;

private:
    std::vector<std::shared_ptr<EventListener>> m_listeners;
};

}

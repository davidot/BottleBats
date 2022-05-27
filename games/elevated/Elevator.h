#pragma once

#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <optional>
#include <functional>
#include "generation/Generation.h"
#include "Types.h"

namespace Elevated {


struct Passenger {
    PassengerID id;
    Height from;
    Height to;
    GroupID group;
    Capacity capacity;

    Passenger(PassengerID id_, PassengerBlueprint blueprint)
        : id(id_)
        , from(blueprint.from)
        , to(blueprint.to)
        , group(blueprint.group)
        , capacity(blueprint.capacity)
    {
    }

    bool operator==(Passenger const& rhs) const {
        return id == rhs.id && from == rhs.from && to == rhs.to && group == rhs.group && capacity == rhs.capacity;
    }
};


    class ElevatorState {
public:
    struct TravellingPassenger {
        PassengerID id;
        Height to;
        Capacity capacity;
    };

    enum class State {
        Stopped,
        Travelling,
        DoorsOpening,
        DoorsOpen,
        DoorsClosing
    };


    ElevatorID const id;
    GroupID const group_id;
    Capacity const max_capacity;
    Height const speed;
    constexpr static Time door_opening_time = 1;
    constexpr static Time door_closing_time = 1;

    enum class ElevatorUpdateResult {
        Nothing,
        DoorsOpened,
        DoorsClosed,
    };

    ElevatorUpdateResult update(Time steps);
    void set_target(Height floor);

    [[nodiscard]] std::vector<TravellingPassenger> const& passengers() const {
        return m_passengers;
    }

    ElevatorState(ElevatorID id_, BuildingBlueprint::Elevator const& elevator, Height initial_height)
        : id(id_)
        , group_id(elevator.group)
        , max_capacity(elevator.max_capacity)
        , speed(elevator.speed)
        , m_height(initial_height)
        , m_target_height(initial_height)
    {
    }

    [[nodiscard]] std::optional<Time> time_until_next_event() const;
    [[nodiscard]] State current_state() const { return m_state; }
    [[nodiscard]] Height target_height() const { return m_target_height; }
    [[nodiscard]] Height height() const { return m_height; }
    [[nodiscard]] Capacity filled_capacity() const;

    struct TransferredPassengers {
        std::vector<ElevatorID> dropped_off_passengers;
        std::vector<Passenger> picked_up_passengers;
    };

    using PassengerCallback = std::function<bool(Passenger const&)>;

    TransferredPassengers transfer_passengers(std::vector<Passenger>& waiting_passengers, PassengerCallback const& callback = [](auto&) { return true; });

private:
    Height m_height{0};
    Height m_target_height{0};
    State m_state = State::Stopped;
    std::vector<TravellingPassenger> m_passengers;

    Time m_time_until_next_state { 0 };

    Time time_for_distance(Height distance) const {
        return distance / speed;
    }

    Time distance_for_time(Time steps) const {
        return speed * steps;
    }

    void pickup_passengers(std::vector<Passenger>& waiting_passengers, TransferredPassengers&, Capacity capacity_left, PassengerCallback const& callback);
    Capacity dropoff_passengers(TransferredPassengers&);
    void move_to_target(Height distance);
};

}

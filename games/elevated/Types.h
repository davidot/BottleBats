#pragma once

#include <cstdint>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <optional>

namespace Elevated {

using ElevatorID = uint32_t;
using PassengerID = uint32_t;
using GroupID = uint32_t;
using Height = uint32_t;
using Time = uint32_t;

constexpr Height distance_between(Height one, Height two) {
    if (one > two)
        return one - two;
    return two - one;
}

static_assert(uint32_t{} == 0);

struct Passenger {
    PassengerID id;
    Height from;
    Height to;
    GroupID group;
};

class ElevatorState {
public:
    struct TravellingPassenger {
        PassengerID id;
        Height to;
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

    ElevatorState(ElevatorID id_, GroupID group_id_, Height initial_height)
        : id(id_), group_id(group_id_), m_height(initial_height), m_target_height(initial_height)
    {
    }

    [[nodiscard]] std::optional<Time> time_until_next_event() const;
    [[nodiscard]] State current_state() const { return m_state; }
    [[nodiscard]] Height target_height() const { return m_target_height; }
    [[nodiscard]] Height height() const { return m_height; }

    struct TransferredPassengers {
        std::vector<TravellingPassenger> dropped_off_passengers;
        std::vector<Passenger> picked_up_passengers;
    };

    TransferredPassengers transfer_passengers(std::vector<Passenger>& waiting_passengers);

private:
    Height m_height;
    Height m_target_height;
    State m_state = State::Stopped;
    std::vector<TravellingPassenger> m_passengers;

    Time m_time_until_next_state { 0 };

    constexpr static Time time_for_distance(Height distance) {
        return distance;
    }

    constexpr static Time distance_for_time(Time steps) {
        return steps;
    }

    void pickup_passengers(std::vector<Passenger>& waiting_passengers, TransferredPassengers&);
    void dropoff_passengers(TransferredPassengers&);
    void move_to_target(Height distance);
};

}

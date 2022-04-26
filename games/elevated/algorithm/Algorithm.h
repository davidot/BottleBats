#pragma once

#include "../Types.h"
#include "../Elevator.h"
#include "../Building.h"

namespace Elevated {


class AlgorithmInput {
public:
    static AlgorithmInput new_request(Height at, size_t index);
    static AlgorithmInput elevator_closed_doors(ElevatorID);
    static AlgorithmInput timer_fired();

    enum class Type {
        NewRequestMade,
        ElevatorClosedDoors,
        TimerFired,
    };

    Type type() const { return m_type; }
    ElevatorID elevator_id() const;
    Passenger const& request(BuildingState const&) const;
private:
    AlgorithmInput() = default;

    Type m_type;
    ElevatorID m_elevator_id = -1;
    Height request_height = -1;
    size_t request_index = -1;
};

class AlgorithmResponse {
public:
    enum class Type {
        MoveElevator,
        SetTimer
    };

    static AlgorithmResponse set_timer_at(Time timer_at);
    static AlgorithmResponse move_elevator_to(ElevatorID, Height);

    Type type() const { return m_type; }

    Time timer_should_fire_at() const;

    ElevatorID elevator_to_move() const;
    Height elevator_target() const;

private:
    AlgorithmResponse() = default;

    Type m_type;
    Time m_timer_at;
    ElevatorID m_elevator_id;
    Height m_target;
};


class ElevatedAlgorithm {
public:
    enum class ScenarioAccepted {
        Yes,
        No
    };

    virtual ScenarioAccepted accept_scenario_description(BuildingGenerationResult const& building) = 0;

    virtual std::vector<AlgorithmResponse> on_inputs(Time at, BuildingState const& building, std::vector<AlgorithmInput> inputs) = 0;
};

}

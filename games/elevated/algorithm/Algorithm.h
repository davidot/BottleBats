#pragma once

#include "../Types.h"
#include "../Elevator.h"
#include "../Building.h"
#include <variant>

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
    Height request_height() const;
    size_t request_index() const;
private:
    AlgorithmInput() = default;

    Type m_type;
    ElevatorID m_elevator_id = -1;
    Height m_request_height = -1;
    size_t m_request_index = -1;
};

class AlgorithmResponse {
public:
    enum class Type {
        MoveElevator,
        SetTimer,
        AlgorithmFailed,
        AlgorithmMisbehaved,
    };

    static AlgorithmResponse set_timer_at(Time timer_at);
    static AlgorithmResponse move_elevator_to(ElevatorID, Height);
    static AlgorithmResponse algorithm_failed(std::vector<std::string> messages);
    static AlgorithmResponse algorithm_misbehaved(std::vector<std::string> messages);

    Type type() const { return m_type; }

    Time timer_should_fire_at() const;

    ElevatorID elevator_to_move() const;
    Height elevator_target() const;

    std::vector<std::string> const& messages() const;
private:
    AlgorithmResponse() = default;

    struct ElevatorMove {
        ElevatorID id;
        Height target;
    };

    Type m_type = Type::MoveElevator;
    std::variant<Time, ElevatorMove, std::vector<std::string>> m_data;
};


class ElevatedAlgorithm {
public:
    virtual ~ElevatedAlgorithm() = default;

    struct ScenarioAccepted {
        enum class Type {
            Accepted,
            Rejected,
            Failed
        };
        Type type;
        std::vector<std::string> messages;

        ScenarioAccepted() noexcept;

        static ScenarioAccepted accepted();
        static ScenarioAccepted failed(std::vector<std::string>);
        static ScenarioAccepted rejected(std::vector<std::string>);
    private:
        ScenarioAccepted(Type, std::vector<std::string> messages);
    };

    virtual ScenarioAccepted accept_scenario_description(BuildingGenerationResult const& building) = 0;

    // FIXME: Maybe just handle this here directly? Instead of returning a function.
    virtual std::optional<ElevatorState::PassengerCallback> on_doors_open(Time, ElevatorID, BuildingState const&) { return std::nullopt; };

    virtual std::vector<AlgorithmResponse> on_inputs(Time at, BuildingState const& building, std::vector<AlgorithmInput> inputs) = 0;
};

}

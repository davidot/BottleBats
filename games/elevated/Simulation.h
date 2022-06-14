#pragma once

#include <memory>
#include "generation/Generation.h"
#include "Building.h"
#include "algorithm/Algorithm.h"

namespace Elevated {


struct SimulatorResult {
    enum class Type {
        SuccessFull,
        GenerationFailed,
        RequestGenerationFailed,
        AlgorithmRejected,
        AlgorithmMisbehaved,
        AlgorithmFailed,
        NoNextEvent,
        FailedToResolveAllRequests,
        Starting,
        Running,
    };

    Type type = Type::SuccessFull;
    std::vector<std::string> output_messages;

    bool is_in_progress() const {
        return type == Type::Starting || type == Type::Running;
    }
};

class Simulation {
public:
    Simulation(std::unique_ptr<ScenarioGenerator> generator, std::unique_ptr<ElevatedAlgorithm> algorithm);

    template<typename ListenerType, typename... Args>
    std::shared_ptr<ListenerType> construct_and_add_listener(Args... args) {
        auto listener = std::make_shared<ListenerType>(std::forward<Args>(args)...);
        add_listener(listener);
        return listener;
    }

    void add_listener(std::shared_ptr<EventListener> listener) { m_event_distributor.add_listener(std::move(listener)); }
    bool remove_listener(EventListener* listener) { return m_event_distributor.remove_listener(listener); }

    ScenarioGenerator& generator() { return *m_generator; }
    ElevatedAlgorithm& algorithm() { return *m_algorithm; }

    BuildingState const& building() const { return m_building; }

    SimulatorResult run_full_simulation();

    SimulatorResult result() const;

    enum class SimulationDone {
        Yes,
        No
    };
    SimulationDone tick();
private:
    bool setup_for_run();

    std::unique_ptr<ScenarioGenerator> m_generator;
    std::unique_ptr<ElevatedAlgorithm> m_algorithm;
    BuildingState m_building;
    EventDistributor m_event_distributor;

    SimulatorResult m_result{SimulatorResult::Type::Starting, {}};

    Time m_last_requests = 0;
    std::optional<Time> m_next_timer = 0;
};

}

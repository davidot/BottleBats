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
        FailedToResolveAllRequests
    };

    Type type = Type::SuccessFull;
    std::vector<std::string> output_messages;
};

class Simulation {
public:
    Simulation(std::unique_ptr<ScenarioGenerator> generator, std::unique_ptr<ElevatedAlgorithm> algorithm);

    void add_listener(std::shared_ptr<EventListener> listener) { m_event_distributor.add_listener(std::move(listener)); }
    bool remove_listener(EventListener* listener) { return m_event_distributor.remove_listener(listener); }

    ScenarioGenerator& generator() { return *m_generator; }
    ElevatedAlgorithm& algorithm() { return *m_algorithm; }

    SimulatorResult run();

private:
    bool setup_for_run();

    std::unique_ptr<ScenarioGenerator> m_generator;
    std::unique_ptr<ElevatedAlgorithm> m_algorithm;
    BuildingState m_building;
    EventDistributor m_event_distributor;

    std::optional<SimulatorResult> result;
};

}

#pragma once

#include <memory>
#include "generation/Generation.h"
#include "Building.h"
#include "algorithm/Algorithm.h"

namespace Elevated {

class Simulator {

    Simulator(std::unique_ptr<ScenarioGenerator> generator, std::unique_ptr<ElevatedAlgorithm> algorithm)
        : m_generator(std::move(generator))
        , m_algorithm(std::move(algorithm))
    {
    }

    void add_listener(std::shared_ptr<EventListener> listener) { m_distributor.add_listener(std::move(listener)); }
    bool remove_listener(EventListener* listener) { return m_distributor.remove_listener(listener); }

private:
    std::unique_ptr<ScenarioGenerator> m_generator;
    std::unique_ptr<ElevatedAlgorithm> m_algorithm;
    BuildingState building;
    EventDistributor m_distributor;
};

}

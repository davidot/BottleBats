#pragma once

#include "Algorithm.h"
namespace Elevated {

class CyclingAlgorithm : public ElevatedAlgorithm {
public:
    virtual ScenarioAccepted accept_scenario_description(const BuildingGenerationResult& building) override;
    virtual std::vector<AlgorithmResponse> on_inputs(Time at, const BuildingState& building, std::vector<AlgorithmInput> inputs) override;

private:
    std::unordered_map<Height, Height> m_next_height;
    bool m_running = false;
};

}

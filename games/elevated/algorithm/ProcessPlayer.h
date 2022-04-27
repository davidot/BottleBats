#pragma once

#include "../../../util/Process.h"
#include "Algorithm.h"
namespace Elevated {

class ProcessAlgorithm: public ElevatedAlgorithm {
public:
    ScenarioAccepted accept_scenario_description(const BuildingGenerationResult& building) override;
    std::vector<AlgorithmResponse> on_inputs(Time at, const BuildingState& building, std::vector<AlgorithmInput> inputs) override;

private:
    std::unique_ptr<util::SubProcess> m_process;
};

}

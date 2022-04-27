#pragma once

#include "../../../util/Process.h"
#include "Algorithm.h"
namespace Elevated {

class ProcessAlgorithm: public ElevatedAlgorithm {
public:
    explicit ProcessAlgorithm(std::vector<std::string> command);

    ScenarioAccepted accept_scenario_description(const BuildingGenerationResult& building) override;
    std::vector<AlgorithmResponse> on_inputs(Time at, const BuildingState& building, std::vector<AlgorithmInput> inputs) override;

private:
    std::unique_ptr<util::SubProcess> m_process;
    std::vector<std::string> m_command;
};

}

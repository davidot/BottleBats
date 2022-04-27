#include "ProcessPlayer.h"

namespace Elevated {

ElevatedAlgorithm::ScenarioAccepted ProcessAlgorithm::accept_scenario_description(BuildingGenerationResult const& building)
{
    return ScenarioAccepted::No;
}

std::vector<AlgorithmResponse> ProcessAlgorithm::on_inputs(Time at, BuildingState const& building, std::vector<AlgorithmInput> inputs)
{
    return std::vector<AlgorithmResponse>();
}


}


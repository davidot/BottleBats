#include "ProcessAlgorithm.h"
#include "../../../util/Assertions.h"

namespace Elevated {

ProcessAlgorithm::ProcessAlgorithm(std::vector<std::string> command)
{
    ASSERT(!command.empty());
    m_command = command;
}

ElevatedAlgorithm::ScenarioAccepted ProcessAlgorithm::accept_scenario_description(BuildingGenerationResult const&)
{
    return ScenarioAccepted::No;
}

std::vector<AlgorithmResponse> ProcessAlgorithm::on_inputs(Time, BuildingState const&, std::vector<AlgorithmInput>)
{
    return {};
}


}

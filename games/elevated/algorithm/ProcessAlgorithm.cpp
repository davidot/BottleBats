#include "ProcessAlgorithm.h"
#include "../../../util/Assertions.h"

namespace Elevated {

ProcessAlgorithm::ProcessAlgorithm(std::vector<std::string> command)
{
    ASSERT(!command.empty());
    m_command = command;
    auto proc_or_fail = util::SubProcess::create(move(command));
    if (!proc_or_fail)
        return;

    m_process = std::move(proc_or_fail);
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

#pragma once

#include "../../../util/Process.h"
#include "Algorithm.h"
#include <sstream>

namespace Elevated {

class ProcessAlgorithm: public ElevatedAlgorithm {
public:
    enum class InfoLevel {
        Full,
        High,
        Low,
        Minimal
    };

    explicit ProcessAlgorithm(std::vector<std::string> command, InfoLevel);
    ProcessAlgorithm(ProcessAlgorithm const&) = delete;
    ProcessAlgorithm& operator=(ProcessAlgorithm const&) = delete;
    ~ProcessAlgorithm();

    ScenarioAccepted accept_scenario_description(BuildingGenerationResult const& building) override;
    std::vector<AlgorithmResponse> on_inputs(Time at, BuildingState const& building, std::vector<AlgorithmInput> inputs) override;

    static void write_building(BuildingGenerationResult const& building, std::ostringstream& stream);

private:
    std::unique_ptr<util::SubProcess> m_process;
    std::vector<std::string> m_command;
    InfoLevel m_info_level;
};

}

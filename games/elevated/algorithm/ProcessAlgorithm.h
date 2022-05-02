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

    explicit ProcessAlgorithm(std::vector<std::string> command, InfoLevel, util::SubProcess::StderrState = util::SubProcess::StderrState::Ignored);
    ProcessAlgorithm(ProcessAlgorithm const&) = delete;
    ProcessAlgorithm& operator=(ProcessAlgorithm const&) = delete;
    ~ProcessAlgorithm();

    ScenarioAccepted accept_scenario_description(BuildingGenerationResult const& building) override;
    std::vector<AlgorithmResponse> on_inputs(Time at, BuildingState const& building, std::vector<AlgorithmInput> inputs) override;

    static void write_building(BuildingGenerationResult const& building, std::ostringstream& stream);

    void write_elevator_closed(ElevatorState const&, std::ostringstream& stream) const;
    void write_new_request(Passenger const&, std::ostringstream&) const;
    bool should_write_new_request(BuildingState const&, Height target, size_t index);

    std::string make_command_string() const;
private:
    std::unique_ptr<util::SubProcess> m_process;
    std::vector<std::string> m_command;
    InfoLevel m_info_level;
    util::SubProcess::StderrState m_stderr_handling;
};

}

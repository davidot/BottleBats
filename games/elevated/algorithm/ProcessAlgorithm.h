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

    explicit ProcessAlgorithm(std::vector<std::string> command, InfoLevel, util::SubProcess::StderrState = util::SubProcess::StderrState::Ignored, std::string working_directory = "");
    ProcessAlgorithm(ProcessAlgorithm const&) = delete;
    ProcessAlgorithm& operator=(ProcessAlgorithm const&) = delete;
    ~ProcessAlgorithm();

    ScenarioAccepted accept_scenario_description(BuildingGenerationResult const& building) override;
    std::optional<ElevatorState::PassengerCallback> on_doors_open(Time time_1, ElevatorID id, BuildingState const& state) override;
    std::vector<AlgorithmResponse> on_inputs(Time at, BuildingState const& building, std::vector<AlgorithmInput> inputs) override;

    static void write_building(BuildingGenerationResult const& building, std::ostringstream& stream);

    void write_elevator_base(ElevatorState const& elevator, std::ostringstream& stream) const;
    void write_elevator_closed(BuildingState const&, ElevatorID, std::ostringstream& stream) const;
    void write_new_request(Passenger const&, std::ostringstream&) const;
    bool should_write_new_request(BuildingState const&, Height target, size_t index);

    std::string make_command_string() const;
private:
    std::unique_ptr<util::SubProcess> m_process;
    std::vector<std::string> m_command;
    InfoLevel m_info_level;
    util::SubProcess::StderrState m_stderr_handling;
    std::string m_working_directory;
    enum class PassengerFilter {
        UpOnly,
        DownOnly
    };
    std::unordered_map<ElevatorID, PassengerFilter> m_filters;
};

}

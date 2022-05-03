#pragma once

#include <elevated/algorithm/Algorithm.h>
#include <memory>
#include <string>

namespace BBServer {

std::unique_ptr<Elevated::ElevatedAlgorithm> algorithm_from_command(std::string);
std::unique_ptr<Elevated::ScenarioGenerator> scenario_from_command(std::string);

struct SimulationResult {
    bool rejected {false};
    bool failed {false};
    std::vector<std::string> messages;

    double avg_wait_time {0};
    Elevated::Time max_wait_time {0};

    double avg_travel_time {0};
    Elevated::Time max_travel_time{0};

    Elevated::Time total_time{0};
    uint64_t power_usage{0};
};

SimulationResult run_simulation(std::unique_ptr<Elevated::ElevatedAlgorithm>, std::unique_ptr<Elevated::ScenarioGenerator>);

void run_and_store_simulation(uint32_t bot_id, uint32_t case_id);

}

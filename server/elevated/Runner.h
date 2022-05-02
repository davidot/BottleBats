#pragma once

#include <elevated/algorithm/Algorithm.h>
#include <memory>
#include <string>

namespace BBServer {

std::unique_ptr<Elevated::ElevatedAlgorithm> algorithm_from_command(std::string);
std::unique_ptr<Elevated::ScenarioGenerator> scenario_from_command(std::string);

void run_simulation(std::unique_ptr<Elevated::ElevatedAlgorithm>, std::unique_ptr<Elevated::ScenarioGenerator>);

}

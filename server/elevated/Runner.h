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

    struct ResultStats {
        std::string name;
        std::variant<double, uint64_t, uint32_t> value;

        ResultStats(std::string name_, double val)
            : name(std::move(name_))
            , value(val)
        {
        }

        ResultStats(std::string name_, uint64_t val)
            : name(std::move(name_))
            , value(val)
        {
        }

        ResultStats(std::string name_, uint32_t val)
            : name(std::move(name_))
            , value(val)
        {
        }
    };

    std::vector<ResultStats> stats;

    template<typename ValueType>
    void add_stat(std::string name, ValueType value) {
        stats.emplace_back(std::move(name), value);
    }
};

SimulationResult run_simulation(std::unique_ptr<Elevated::ElevatedAlgorithm>, std::unique_ptr<Elevated::ScenarioGenerator>);

void run_and_store_simulation(uint32_t bot_id, uint32_t case_id);

}

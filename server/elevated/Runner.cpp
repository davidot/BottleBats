#include "Runner.h"
#include "../../util/Assertions.h"
#include <elevated/Simulation.h>
#include <elevated/stats/PassengerStats.h>
#include <elevated/stats/PowerStatsListener.h>
#include <elevated/stats/SpecialEventsListener.h>

namespace BBServer {

void run_simulation(std::unique_ptr<Elevated::ElevatedAlgorithm> algorithm, std::unique_ptr<Elevated::ScenarioGenerator> generator)
{
    static std::unordered_map<Elevated::SimulatorResult::Type, std::string> type_to_message {
        { Elevated::SimulatorResult::Type::AlgorithmFailed, "Algorithm failed/timed out" },
        { Elevated::SimulatorResult::Type::AlgorithmMisbehaved, "Algorithm made illegal move" },
        { Elevated::SimulatorResult::Type::AlgorithmRejected, "Algorithm rejected scenario" },
        { Elevated::SimulatorResult::Type::FailedToResolveAllRequests, "Algorithm did not resolve requests in time" },
        { Elevated::SimulatorResult::Type::GenerationFailed, "The scenario generation failed" },
        { Elevated::SimulatorResult::Type::RequestGenerationFailed, "The request generation failed" },
        { Elevated::SimulatorResult::Type::NoNextEvent, "Simulation ended but not all requests were resolved" },
    };
    ASSERT(algorithm);
    ASSERT(generator);
    Elevated::Simulation simulation{std::move(generator), std::move(algorithm)};

    auto passenger_stats = simulation.construct_and_add_listener<Elevated::PassengerStatsListener>();
    auto power_stats = simulation.construct_and_add_listener<Elevated::PowerStatsListener>();
    auto special_stats = simulation.construct_and_add_listener<Elevated::SpecialEventsListener>();

    auto result = simulation.run();

    if (result.type == Elevated::SimulatorResult::Type::SuccessFull) {

    } else if (result.type == Elevated::SimulatorResult::Type::AlgorithmRejected) {

    } else {

    }
}

}

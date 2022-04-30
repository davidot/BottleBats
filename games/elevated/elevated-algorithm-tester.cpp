#include "elevated/generation/FullGenerators.h"
#include "elevated/stats/MetaListener.h"
#include "elevated/stats/PassengerStats.h"
#include <ctime>
#include <elevated/Simulation.h>
#include <elevated/algorithm/ProcessAlgorithm.h>
#include <iostream>
#include <string>
#include <vector>

using namespace Elevated;

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cout << "Please give bot run command\n";
        return 1;
    }

    std::vector<std::string> command;

    bool in_flags = true;

    for (int i = 1; i < argc; ++i) {
        std::string val = argv[i];

        if (in_flags) {
            if (val == "--groups") {
                if (i == argc - 1)
                    std::cout << "Must give amount after --groups\n";
                i++;
//                std::string group_val = argv[i];
//                int temp = std::stoi(group_val);

                continue;
            }
        }

        in_flags = false;
        command.push_back(val);
    }

    if (command.empty()) {
        std::cout << "Must give command (after options)\n";
        return 1;
    }


//    srand(time(nullptr));
//
//    uint32_t seed = rand();

    std::vector<std::pair<size_t, std::vector<PassengerBlueprint>>> requests = {
        { 0, { { 0, 10, 0 }, {10, 0, 0}, {5, 15, 0}, {5, 10, 0}, {0, 15, 0} } },
    };

    requests.reserve(1667);
    for (size_t t = 5; t < 50010; t += 43) {
        std::vector<PassengerBlueprint> at;

        if (t % 2 == 0)
            at.push_back({0, 15, 0});

        if (t % 3 == 0)
            at.push_back({10, 5, 0});

        if (t % 2 && t > 1000 && t < 6000)
            at.push_back({5, 0, 0});

        if (t % 4 == 0) {
            at.push_back({10, 0, 0});
            at.push_back({0, 10, 0});
        }

        if (!at.empty())
            requests.emplace_back(t, std::move(at));
    }

    std::unique_ptr<ScenarioGenerator> generator = std::make_unique<HardcodedScenarioGenerator>(
        std::vector<std::pair<size_t, std::vector<Height>>> { { 3, { 0, 5, 10, 15, 5000 } } },
        std::move(requests));

    std::unique_ptr<ElevatedAlgorithm> algorithm = std::make_unique<ProcessAlgorithm>(command, ProcessAlgorithm::InfoLevel::Low);

    Simulation simulation { std::move(generator), std::move(algorithm) };

    auto passenger_stats_listener = simulation.construct_and_add_listener<PassengerStatsListener>();

    auto meta_listener = simulation.construct_and_add_listener<MetaListener>();

    auto result = simulation.run();

    switch (result.type) {
    case SimulatorResult::Type::SuccessFull:
        std::cout << "Ran complete simulation in " << simulation.building().current_time() << " steps\n";
        std::cout << "Which was " << meta_listener->ticks() << " tick of the simulation and " << meta_listener->events() << " events\n";
        std::cout << "Max waiting time " << passenger_stats_listener->max_wait_times() << '\n';
        std::cout << "Max travel time " << passenger_stats_listener->max_travel_times() << '\n';
        break;
    case SimulatorResult::Type::GenerationFailed:
        std::cout << "Something went wrong in the generation of the scenario\n";
        break;
    case SimulatorResult::Type::RequestGenerationFailed:
        std::cout << "Something went wrong in the generation of the requests\n";
        break;
    case SimulatorResult::Type::AlgorithmRejected:
        std::cout << "Your algorithm rejected this case\n";
        break;
    case SimulatorResult::Type::AlgorithmMisbehaved:
        std::cout << "Your algorithm did something which was not allowed\n";
        break;
    case SimulatorResult::Type::AlgorithmFailed:
        std::cout << "Your algorithm was too slow to/didn't respond properly\n";
        break;
    case SimulatorResult::Type::NoNextEvent:
        std::cout << "There were still requests left to handle but there was nothing to do, (i.e. people are waiting somewhere)\n";
        break;
    case SimulatorResult::Type::FailedToResolveAllRequests:
        std::cout << "There was too long a period after the last new request where nothing happened\n";
        break;
    }

    for (auto& message : result.output_messages)
        std::cout << "  " << message << '\n';

}

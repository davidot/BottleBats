#include <elevated/generation/FullGenerators.h>
#include <elevated/stats/MetaListener.h>
#include <elevated/stats/PassengerStats.h>
#include <elevated/stats/PowerStatsListener.h>
#include <elevated/stats/SpecialEventsListener.h>
#include <ctime>
#include <elevated/Simulation.h>
#include <elevated/algorithm/ProcessAlgorithm.h>
#include <elevated/generation/factory/NamedScenarios.h>
#include <elevated/generation/factory/StringSettings.h>
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
    std::string input = "named-scenario(h1)";
    std::string cwd = "";

    bool in_flags = true;

    for (int i = 1; i < argc; ++i) {
        std::string val = argv[i];

        if (in_flags) {
            if (val == "--gen") {
                if (i == argc - 1) {
                    std::cout << "Must give generator name after --gen\n";
                    return 1;
                }
                i++;

                input = argv[i];
                continue;
            } else if (val == "--cwd") {
                if (i == argc - 1) {
                    std::cout << "Must give working directory after --cwd\n";
                    return 1;
                }
                i++;

                cwd = argv[i];
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

    auto scenario_result = parse_scenario(input, rand());

    auto generator = std::move(scenario_result.generator);


    if (!generator) {
        std::cerr << "Invalid generator\n";
        for (auto& s : scenario_result.errors)
            std::cerr << s << '\n';
        return 1;
    }

    for (auto& s : scenario_result.errors)
        std::cerr << s << '\n';


    std::unique_ptr<ElevatedAlgorithm> algorithm = std::make_unique<ProcessAlgorithm>(command, ProcessAlgorithm::InfoLevel::Low, util::SubProcess::StderrState::Forwarded, std::move(cwd));

    Simulation simulation { std::move(generator), std::move(algorithm) };

    auto passenger_stats_listener = simulation.construct_and_add_listener<PassengerStatsListener>();
    auto power_stats = simulation.construct_and_add_listener<PowerStatsListener>();
    auto special_stats = simulation.construct_and_add_listener<SpecialEventsListener>();

    auto meta_listener = simulation.construct_and_add_listener<MetaListener>();

    auto result = simulation.run();

    switch (result.type) {
    case SimulatorResult::Type::SuccessFull:
        std::cout << "Ran complete simulation in " << simulation.building().current_time() << " steps\n";
        std::cout << "Which was " << meta_listener->ticks() << " tick of the simulation and " << meta_listener->events() << " events\n";
        std::cout << "Max waiting time: " << passenger_stats_listener->max_wait_times() << " avg: " << passenger_stats_listener->average_wait_time() << '\n';
        std::cout << "Max travel time: " << passenger_stats_listener->max_travel_times() << '\n';
        std::cout << "Max time door opened: " << passenger_stats_listener->max_times_door_opened() << '\n';
        std::cout << "Power: Doors opened: " << power_stats->times_door_opened() << " total distance travelled: " << power_stats->total_distance_travelled() << " time stopped with passengers: " << power_stats->time_stopped_with_passengers() << '\n';
        std::cout << "Roller coaster events: " << special_stats->total_roller_coaster_events()  << '\n';
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

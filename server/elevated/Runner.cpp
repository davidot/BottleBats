#include "Runner.h"
#include "../../util/Assertions.h"
#include "../database/ConnectionPool.h"
#include "elevated/algorithm/CyclingAlgorithm.h"
#include "elevated/algorithm/ProcessAlgorithm.h"
#include "elevated/generation/FullGenerators.h"
#include "elevated/generation/factory/StringSettings.h"
#include <crow/json.h>
#include <elevated/Simulation.h>
#include <elevated/stats/PassengerStats.h>
#include <elevated/stats/PowerStatsListener.h>
#include <elevated/stats/SpecialEventsListener.h>
#include <iostream>
#include <pqxx/connection>
#include <pqxx/transaction>

namespace BBServer {

std::unique_ptr<Elevated::ElevatedAlgorithm> algorithm_from_command(std::string name)
{
    auto separator_index = name.find(':');
    if (separator_index == std::string::npos) {
        return nullptr;
    }


    auto type = name.substr(0, separator_index);
    auto details = name.substr(separator_index + 1);



    if (type == "internal") {
        if (details == "simple-cycle") {
            return std::make_unique<Elevated::CyclingAlgorithm>();
        }
    } else if (type == "podman") {
        return std::make_unique<Elevated::ProcessAlgorithm>(std::vector<std::string> {
            "podman", "run",
            "--network=none", "--cpus=1.0", "--memory=256m",
            "--cap-drop=all", "--rm", "--interactive",
            std::string(details)
        }, Elevated::ProcessAlgorithm::InfoLevel::Low);
    }

    return nullptr;
}

std::unique_ptr<Elevated::ScenarioGenerator> scenario_from_command(std::string name)
{
    auto result = Elevated::parse_scenario(name, 783675);
    if (!result.generator) {
        std::cerr << "Scenario failed: \n";
        for (auto& line : result.errors)
            std::cerr << "    " << line << '\n';
        throw "failed scenario!";
    }
    return std::move(result.generator);
}

SimulationResult run_simulation(std::unique_ptr<Elevated::ElevatedAlgorithm> algorithm, std::unique_ptr<Elevated::ScenarioGenerator> generator)
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

    SimulationResult full_result{};
    full_result.total_time = simulation.building().current_time();
    full_result.messages = std::move(result.output_messages);

    if (result.type == Elevated::SimulatorResult::Type::SuccessFull) {
        full_result.avg_wait_time = passenger_stats->average_wait_time();
        full_result.max_wait_time = passenger_stats->max_wait_times();
        full_result.max_travel_time = passenger_stats->max_travel_times();
        full_result.power_usage = power_stats->time_stopped_with_passengers() + power_stats->times_door_opened() + power_stats->total_distance_travelled();
        full_result.avg_wait_time = passenger_stats->average_wait_time();
        full_result.avg_travel_time = passenger_stats->average_travel_time();
    } else if (result.type == Elevated::SimulatorResult::Type::AlgorithmRejected) {
        full_result.rejected = true;
    } else {
        full_result.failed = true;
        full_result.messages.push_back(type_to_message[result.type]);
    }
    return full_result;
}

void run_and_store_simulation(uint32_t bot_id, uint32_t case_id)
{
    bool fail = true;
    std::string bot_command;
    std::string case_command;
    uint64_t run_id;

    ConnectionPool::run_on_temporary_connection([&](pqxx::connection& connection) {
        pqxx::work transaction{connection};
        connection.prepare("SELECT command FROM elevated_bots WHERE bot_id = $1 AND running_cases LIMIT 1");
        auto result = transaction.exec_prepared("", bot_id);

        if (result.empty())
            return;

        bot_command = result[0][0].c_str();

        connection.prepare("SELECT command FROM elevated_cases WHERE case_id = $1 AND enabled LIMIT 1");
        auto case_result = transaction.exec_prepared("", case_id);

        if (case_result.empty())
            return;

        case_command = case_result[0][0].c_str();

        connection.prepare("INSERT INTO elevated_run(bot_id, case_id) VALUES ($1, $2) RETURNING run_id");
        auto run_result = transaction.exec_prepared1("", bot_id, case_id);
        transaction.commit();
        run_id = run_result[0].as<uint64_t>();

        fail = false;
    });

    if (fail) {
        std::cerr << "Failed to run bot " << bot_id << " on " << case_id << '\n';
        return;
    }

    auto algorithm = algorithm_from_command(bot_command);

    if (!algorithm) {
        ConnectionPool::run_on_temporary_connection([&](pqxx::connection& connection) {
            pqxx::work transaction { connection };
            transaction.exec("UPDATE elevated_bots SET running_cases = FALSE, status = 'Disabled: Command is invalid' WHERE bot_id = " + std::to_string(bot_id));
            transaction.commit();
        });
        std::cerr << "Invalid bot: " << bot_command << " disabled and skipping this\n";
        return;
    }

    auto generator = scenario_from_command(case_command);

    if (!generator) {
        ConnectionPool::run_on_temporary_connection([&](pqxx::connection& connection) {
            pqxx::work transaction { connection };
            transaction.exec("UPDATE elevated_cases SET running_cases = FALSE WHERE case_id = " + std::to_string(case_id));
            transaction.commit();
        });
        std::cerr << "Invalid generator: " << case_command << " disabled and skipping this\n";
        return;
    }

    auto result = run_simulation(std::move(algorithm), std::move(generator));

    bool success = !(result.failed || result.rejected);
    std::string status = result.failed ? "failed" : result.rejected ? "rejected" : "done";

    std::string output;

    if (success) {
        crow::json::wvalue encoded_result;
        encoded_result["avg-wait"] = result.avg_wait_time;
        encoded_result["avg-travel"] = result.avg_travel_time;
        encoded_result["max-wait"] = result.max_wait_time;
        encoded_result["max-travel"] = result.max_travel_time;
        encoded_result["power"] = result.power_usage;
        encoded_result["total-time"] = result.total_time;

        output = encoded_result.dump();
    } else {
        for (auto& message : result.messages)
            output += message + '\n';
    }
    
    ConnectionPool::run_on_temporary_connection([&](pqxx::connection& connection) {
        pqxx::work transaction{connection};
        connection.prepare("UPDATE elevated_run SET completed = now(), done = TRUE, success = $2, status = $3, output = $4 WHERE run_id = $1");
        auto run_result = transaction.exec_prepared("", run_id, success, status, output);
        ASSERT(run_result.affected_rows() == 1);
        transaction.commit();
    });
}

}

#include "BotCreator.h"

#include <string>
#include <pqxx/connection>
#include <pqxx/transaction>
#include <iostream>
#include <filesystem>
#include "database/ConnectionPool.h"
#include "../util/Assertions.h"
#include "../util/Process.h"
#include "../games/vijf/Game.h"
#include "../games/vijf/Vijf.h"
#include "ContainerBuilder.h"
#include "database/ConnectionPool.h"
#include <iostream>
#include <pqxx/connection>
#include <pqxx/transaction>
#include <string>

namespace BBServer {

static constexpr std::string_view random_player_command = "internal:random";

bool create_bot_in_container(uint32_t id) {

    std::string filename;
    auto start_status = ConnectionPool::run_on_temporary_connection([&](pqxx::connection& connection) -> std::string {
        pqxx::read_transaction transaction{connection};

        auto result = transaction.exec("SELECT state, command FROM vijf_bots WHERE bot_id = " + std::to_string(id));
        if (result.size() > 1) {
            ASSERT_NOT_REACHED();
        }

        if (result.empty())
            return {};

        filename = result[0][1].c_str();
        return result[0][0].c_str();
    });

    if (start_status.empty()) {
        std::cerr << "Bot " << id << " does not exist?\n";
        return false;
    }

    if (start_status != "Added to database") {
        std::cerr << "Work is/was already done on bot " << id << '\n';
        return false;
    }

    ASSERT(!filename.empty());
    ASSERT(filename.find('.') != std::string::npos);

    auto update_status = [&](std::string const& new_status, bool failed = true) {
        ConnectionPool::run_on_temporary_connection([&](pqxx::connection& connection) {
            pqxx::work transaction{connection};

            connection.prepare("UPDATE vijf_bots SET state = $1, failed = $2 WHERE bot_id = $3");

            auto result = transaction.exec_prepared0("", new_status, failed, id);
            ASSERT(result.affected_rows() == 1);
            transaction.commit();
        });
    };

    update_status("Checking and building file", false);

    std::string file_path = std::string("bots-data/") + std::to_string(id) + std::string("/") + filename;
    std::string container_name = "vijfbot-" + std::to_string(id);

    auto build_result = build_single_file_container(file_path, container_name);

    if (build_result.has_value()) {
        update_status(build_result.value());
        return false;
    }

    auto engine = std::default_random_engine{ 0xb0d1234 };

    std::string run_command = "container:" + container_name;

    std::array<std::string_view, Vijf::player_count> players = {
        random_player_command, random_player_command, random_player_command, random_player_command, random_player_command
    };

    int slow_games = 0;

    const int games_to_play = 10;
    const int max_slow_games = 5;

    for (int i = 0; i < games_to_play; i++) {
        std::string game_num = std::to_string(i + 1);
        if (game_num.size() < 2)
            game_num.insert(0, 1, ' ');

        update_status("Running test games (" + std::to_string(i) + '/' + std::to_string(games_to_play) + ")", false);

        size_t player_position = i % Vijf::player_count;

        players[player_position] = run_command;

        auto initial_data = Vijf::generate_random_start(engine);
        ASSERT(initial_data.hands[player_position].total_cards() > 0);
        auto game_result = Vijf::play_game(initial_data, players);

        auto& player_events = game_result.events[player_position];

        if (Vijf::has_event<Vijf::EventType::ProcessPlayerMisbehaved>(player_events)) {
            update_status("Made illegal move or gave wrong\nGame: " + initial_data.to_string());
            return false;
        }

        if (Vijf::has_event<Vijf::EventType::ProcessPlayerTooSlowToPlay>(player_events) || Vijf::has_event<Vijf::EventType::ProcessPlayerTooSlowToStart>(player_events)) {
            slow_games++;
            if (slow_games >= max_slow_games) {
                update_status("You were too slow to respond in 5 games.");
                return false;
            }
        }

        players[player_position] = random_player_command;
    }

    ConnectionPool::run_on_temporary_connection([&](pqxx::connection& connection) {
        pqxx::work transaction{connection};

        connection.prepare("UPDATE vijf_bots SET state = $1, enabled = TRUE, command = $2 WHERE bot_id = $3");

        auto result = transaction.exec_prepared0("", "Running in tournament", run_command, id);
        ASSERT(result.affected_rows() == 1);
        transaction.commit();
    });

    return true;
}

}

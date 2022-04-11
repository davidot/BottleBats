#include <mutex>
#include <random>
#include <ranges>
#include <pqxx/connection>
#include <pqxx/transaction>
#include <iostream>
#include "GamePlayer.h"
#include "vijf/Vijf.h"
#include "database/ConnectionPool.h"

namespace BBServer {

static std::mutex engine_lock;
static std::mt19937 rng_engine;


void init_engine()
{
    std::lock_guard lock(engine_lock);
    const int N = std::mt19937::state_size * sizeof(std::mt19937::result_type);
    std::random_device source;
    std::random_device::result_type random_data[(N - 1) / sizeof(source()) + 1];
    std::generate(std::begin(random_data), std::end(random_data), std::ref(source));
    std::seed_seq seeds(std::begin(random_data), std::end(random_data));
    rng_engine.seed(seeds);
}

Vijf::StartData generate_random_start()
{
    std::lock_guard lock(engine_lock);
    return Vijf::generate_random_start(rng_engine);
}

//void play_random_game(std::array<std::string, Vijf::player_count> commands)
//{
//    Vijf::StartData start;
//    {
//        std::lock_guard lock(engine_lock);
//        start = Vijf::generate_random_start(rng_engine);
//    }
//
//    std::array<std::string_view, Vijf::player_count> views {};
//    for (int i = 0; i < Vijf::player_count; ++i)
//        views[i] = commands[i];
//
//    play_and_record_game(start, views);
//}

static constexpr std::string_view fallback_command = "internal:random";

void play_and_record_game(Vijf::StartData start_data, std::array<std::optional<uint32_t>, Vijf::player_count> bot_ids)
{
    bool fail = false;
    std::array<std::string, Vijf::player_count> backing_strings;
    std::array<std::string_view, Vijf::player_count> commands = {};
    ConnectionPool::run_on_temporary_connection([&](pqxx::connection& connection) {
        pqxx::read_transaction transaction{connection};
        connection.prepare("SELECT command FROM vijf_bots WHERE bot_id = $1 AND enabled LIMIT 1");

        for (int i = 0; i < Vijf::player_count; ++i) {
            if (!bot_ids[i].has_value()) {
                commands[i] = fallback_command;
                continue;
            }

            auto result = transaction.exec_prepared("", bot_ids[i].value());
            if (result.empty()) {
                std::cerr << "Needed to play a game with " << bot_ids[i].value() << " but does not exist (maybe disabled in the mean time??\n";
                fail = true;
                return;
            }
            backing_strings[i] = result[0][0].c_str();
            commands[i] = backing_strings[i];
        }
    });

    if (fail)
        return;

    std::string start_string = start_data.to_string();
    auto result = Vijf::play_game(std::move(start_data), commands);

    add_game_result({
        bot_ids, start_string, result
    });
}

static std::mutex result_mutex;
static std::vector<CompleteGameResult> pending_results;

void add_game_result(CompleteGameResult new_result)
{
    std::lock_guard lock(result_mutex);
    pending_results.emplace_back(std::move(new_result));
}

void disable_player(pqxx::connection& connection, pqxx::work& transaction, uint32_t bot_id, std::string reason) {
    std::cerr << "Disabling: " << bot_id << " For reason: \n" << reason;
    connection.prepare("UPDATE vijf_bots SET enabled = FALSE, state = $2 WHERE bot_id = $1");
    transaction.exec_prepared("", bot_id, reason);
}

bool write_game_to_database(pqxx::connection& connection, pqxx::work& transaction, CompleteGameResult full_result)
{
    auto& result = full_result.result;
    auto& start_string = full_result.start_string;
    auto& bot_ids = full_result.bot_ids;

    std::ostringstream moves_string;

    for (auto &card : result.moves_made)
        moves_string << card_to_char_repr(card);

    bool any_failed = false;

    if (result.type == Vijf::Results::Type::PlayerMisbehaved) {
        ASSERT(bot_ids[result.player].has_value());
        disable_player(connection, transaction, bot_ids[result.player].value(), "Made invalid move or gave incorrect command.\nFor game: " + start_string + " ; " + moves_string.str());
        any_failed = true;
    }

    for (int i = 0; i < Vijf::player_count; ++i) {
        if (Vijf::has_event<Vijf::EventType::ProcessPlayerMisbehaved>(result.events[i])) {
            ASSERT(bot_ids[i].has_value());
            disable_player(connection, transaction, bot_ids[result.player].value(), "Made invalid move or gave incorrect command.\nFor game: " + start_string + " ; " + moves_string.str());
            any_failed = true;
        }
    }

    if (any_failed)
        return false;

    ASSERT(result.type == Vijf::Results::Type::PlayerWon);

    connection.prepare("INSERT INTO vijf_games (start_data, turn_data, rounds_played, moves_played) VALUES ($1, $2, $3, $4) RETURNING game_id");

    auto game_id_row = transaction.exec_prepared1("", start_string, moves_string.str(), result.rounds_played, result.moves_made.size());

    auto game_id = game_id_row[0].as<uint32_t>();

    connection.prepare("INSERT INTO vijf_game_players (game_id, game_position, bot_id, instadied, game_result, events) VALUES ($1, $2, $3, $4, $5, $6)");

    for (int i = 0; i < Vijf::player_count; ++i) {
        if (!bot_ids[i].has_value())
            continue;

        [[maybe_unused]] auto query_result = transaction.exec_prepared0("", game_id, i, bot_ids[i].value(), result.instadied[i], result.final_rank[i], static_cast<std::underlying_type_t<Vijf::EventType>>(result.events[i]));
        ASSERT(query_result.affected_rows() == 1);
    }

    return true;
}

void flush_results_to_database()
{
    std::vector<CompleteGameResult> new_results;
    {
        std::lock_guard lock(result_mutex);
        if (pending_results.empty())
            return;

        new_results.reserve(pending_results.size());
        std::move(pending_results.begin(), pending_results.end(), std::back_inserter(new_results));
        pending_results.clear();
    }

    ConnectionPool::run_on_temporary_connection([&](pqxx::connection& connection) {
        pqxx::work transaction{connection};
        for (auto& result : new_results)
            write_game_to_database(connection, transaction, result);

        transaction.commit();
    });

}


}

#pragma once

#include <string>
#include <vector>
#include "vijf/Game.h"

namespace BBServer {

void init_engine();

void play_random_game(std::array<std::string, Vijf::player_count> commands);

Vijf::StartData generate_random_start();

struct CompleteGameResult {
    std::array<std::optional<uint32_t>, Vijf::player_count> bot_ids;
    std::string start_string;
    Vijf::Results result;
};

void play_initial_random_games(uint32_t bot_id);

CompleteGameResult play_and_record_game(Vijf::StartData, std::array<std::optional<uint32_t>, Vijf::player_count>);

struct Bot {
    uint32_t id;
    std::string command;
};

std::optional<std::array<Bot, Vijf::player_count>> new_players();
std::optional<std::array<Bot, Vijf::player_count>> top_players();

void play_rotated_game(Vijf::StartData, std::array<Bot, Vijf::player_count> bots);

void add_game_result(CompleteGameResult);
void add_game_results(std::vector<CompleteGameResult>& new_results);

void flush_results_to_database();

}

#pragma once

#include <string>
#include <vector>
#include "vijf/Game.h"

namespace BBServer {

void init_engine();

void play_random_game(std::array<std::string, Vijf::player_count> commands);

Vijf::StartData generate_random_start();

void play_and_record_game(Vijf::StartData, std::array<std::optional<uint32_t>, Vijf::player_count>);

struct CompleteGameResult {
    std::array<std::optional<uint32_t>, Vijf::player_count> bot_ids;
    std::string start_string;
    Vijf::Results result;
};

void add_game_result(CompleteGameResult);

void flush_results_to_database();

}

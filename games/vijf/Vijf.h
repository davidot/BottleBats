#include "Cards.h"
#include "Game.h"

#pragma once

namespace Vijf {

StartData generate_random_start(std::default_random_engine& rng);

StartData generate_random_start_no_died(std::default_random_engine& rng);

Results play_game(StartData data, std::array<std::string_view, player_count> const& player_commands);

}

#include "Cards.h"
#include "Game.h"

#pragma once

namespace Vijf {

template<typename EngineType>
StartData generate_random_start(EngineType& rng);

StartData generate_random_start_no_died(std::default_random_engine& rng);

Results play_game(StartData data, std::array<std::string_view, player_count> const& player_commands);

}

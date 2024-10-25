#pragma once

#include <vector>
#include <string>
#include "InteractiveGameState.h"
#include "PlayerFactory.h"
#include "InteractiveGameTickResult.h"

namespace BBServer {

struct InteractiveGame {
    virtual ~InteractiveGame() { }

    virtual uint32_t get_num_players() const = 0;
    virtual std::vector<std::string> const& available_algortihms() const = 0;
    virtual InteractiveGameState* start_game(std::vector<PlayerFactory> const& setup) const = 0;

    virtual InteractiveGameTickResult tick_interactive_game(InteractiveGameState* game_data) const = 0;
};

}
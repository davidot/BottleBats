#pragma once

#include "../Cards.h"
#include "../Game.h"

namespace Vijf {
class VijfPlayer {
public:
    virtual CardNumber take_turn(GameState const& game_state, std::size_t your_position) = 0;
    virtual ~VijfPlayer() = default;
};

CardNumber random_pick(CardStack const& stack);

}

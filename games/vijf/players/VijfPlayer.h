#pragma once

#include <string_view>
#include <memory>
#include "../Cards.h"
#include "../Game.h"

namespace Vijf {
class VijfPlayer {
public:
    virtual CardNumber take_turn(GameState const& game_state, std::size_t your_position) = 0;
    virtual ~VijfPlayer() = default;

    static std::unique_ptr<VijfPlayer> from_command(std::string_view);
};

CardNumber random_pick(CardStack const& stack, std::minstd_rand& engine);

}

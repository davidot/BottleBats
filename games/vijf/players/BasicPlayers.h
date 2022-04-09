#pragma once

#include "../Cards.h"
#include "VijfPlayer.h"

namespace Vijf {

class LowestFirst : public VijfPlayer {
public:
    CardNumber take_turn(const GameState& game_state, std::size_t your_position) override;
};

class HighestFirst : public VijfPlayer {
public:
    CardNumber take_turn(const GameState& game_state, std::size_t your_position) override;
};

class RandomPlayer : public VijfPlayer {
public:
    RandomPlayer() = default;

    CardNumber take_turn(const GameState& game_state, std::size_t your_position) override;
};

class CheatingPlayer : public VijfPlayer {
public:
    CardNumber take_turn(const GameState& game_state, std::size_t your_position) override;
};

}

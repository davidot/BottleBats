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
    CardNumber take_turn(const GameState& game_state, std::size_t your_position) override;
};

class CheatingPlayer : public VijfPlayer {
public:
    CardNumber take_turn(const GameState& game_state, std::size_t your_position) override;
};

template<typename PlayerOne, uint32_t ChanceOne, typename PlayerTwo, uint32_t ChanceTwo>
class CompoundPlayer : public VijfPlayer {
public:
    CardNumber take_turn(GameState const& game_state, std::size_t your_position) override
    {
        auto value = m_int_dist(game_state.rng_engine);
        if (value <= ChanceOne)
            return m_player_one.take_turn(game_state, your_position);

        return m_player_two.take_turn(game_state, your_position);
    }

private:
    PlayerOne m_player_one{};
    PlayerTwo m_player_two{};
    std::uniform_int_distribution<uint32_t> m_int_dist{0, ChanceOne + ChanceTwo - 1};
};

}

#pragma once

#include "Cards.h"

namespace Vijf {

constexpr int player_count = 5;

struct GameState {
    std::size_t players_alive;
    std::size_t round_number;
    std::array<CardStack const*, player_count> hands;
    CardStack const& discarded_cards;
    OrderedCardStack const& deck; // No cheating!
};

struct Results {
    enum class Type {
        PlayerWon,
        PlayerMisbehaved,
    };
    Type type { Type::PlayerWon };
    std::size_t player;
    int rounds_played = 0;
    std::array<bool, player_count> instadied { false, false, false, false, false };
    std::vector<CardNumber> moves_made;
};

struct StartData {
    CardStack discarded;
    OrderedCardStack deck;
    std::array<CardStack, player_count> hands;

    [[nodiscard]] std::string to_string() const;
};

}

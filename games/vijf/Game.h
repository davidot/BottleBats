#pragma once

#include "Cards.h"

namespace Vijf {

constexpr int player_count = 5;

enum class EventType : uint32_t {
    NoEvents = 0,
    RulesDoubleKill = 1 << 0,
    RulesTripleKill = 1 << 1,
    RulesQuadrupleKill = 1 << 2,
    RanOutOfCards = 1 << 3,
    WonWithNoCards = 1 << 4,
    CouldHaveSavedYourSelf = 1 << 5,
    NoChoiceInCard = 1 << 6,
    AllOfNonSpecial = 1 << 7,
    DiedWithRuleCard = 1 << 8,

    ProcessPlayerToSlowToStart = 1 << 10,
    ProcessPlayerToSlowToPlay = 1 << 11,
    ProcessPlayerMisbehaved = 1 << 12,
};

std::vector<EventType> all_types_in(EventType type);
void add_event(EventType& val, EventType extra);

struct GameState {
    std::size_t players_alive;
    std::size_t round_number;
    std::array<CardStack const*, player_count> hands;
    CardStack const& discarded_cards;
    OrderedCardStack const& deck; // No cheating!

    std::array<EventType, player_count>& events;
    mutable std::minstd_rand rng_engine;
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

    std::array<EventType, player_count> events;
};

struct StartData {
    CardStack discarded;
    OrderedCardStack deck;
    std::array<CardStack, player_count> hands;

    [[nodiscard]] std::string to_string() const;
};

}

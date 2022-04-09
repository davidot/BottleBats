#include "BasicPlayers.h"

namespace Vijf {

Vijf::CardNumber Vijf::LowestFirst::take_turn(Vijf::GameState const& game_state, std::size_t your_position)
{
    auto& hand = *game_state.hands[your_position];
    assert(hand.total_cards() > 0);
    for (auto& card : low_to_high_cards) {
        if (hand.has_card(card))
            return card;
    }
    ASSERT_NOT_REACHED();
}

CardNumber HighestFirst::take_turn(GameState const& game_state, std::size_t your_position)
{
    auto& hand = *game_state.hands[your_position];
    for (auto it = low_to_high_cards.rbegin(); it != low_to_high_cards.rend(); ++it) {
        if (hand.has_card(*it))
            return *it;
    }
    ASSERT_NOT_REACHED();
}

CardNumber RandomPlayer::take_turn(GameState const& game_state, std::size_t your_position)
{
    auto& hand = *game_state.hands[your_position];
    return random_pick(hand, game_state.rng_engine);
}

CardNumber CheatingPlayer::take_turn(GameState const& game_state, std::size_t your_position)
{
    auto next_five = game_state.deck.next_five();
    assert(next_five.has_value());
    assert(next_five < game_state.deck.cards_left());
    auto& hand = *game_state.hands[your_position];
    if (hand.has_card(CardNumber::RuleCard)) {
        if (next_five < game_state.players_alive * 2)
            return CardNumber::RuleCard;
    }

    bool any = false;
    CardNumber highest = CardNumber::King;
    for (auto const& card : low_to_high_cards) {
        if (!hand.has_card(card)) {
            continue;
        }
        auto to_get = number_of_card_to_get(card);
        if (to_get >= next_five) {
            if (!any) {
                return card;
            }
            break;
        }
        highest = card;
        any = true;
    }

    return highest;
}

}

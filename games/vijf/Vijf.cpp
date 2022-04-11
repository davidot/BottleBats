#include <memory>
#include <iostream>
#include "Vijf.h"
#include "players/VijfPlayer.h"
#include "players/ProcessPlayer.h"
#include "players/BasicPlayers.h"
#include <chrono>

namespace Vijf {

bool silent = true;

Results play_game(StartData data, std::array<std::string_view, player_count> const& player_commands)
{

    CardStack discarded_cards = data.discarded;
    OrderedCardStack deck { std::move(data.deck) };
    ASSERT(deck.five_count() > 0);

    struct Player {
        CardStack hand {};
        bool alive { true };
        std::unique_ptr<VijfPlayer> engine { nullptr };
    };

    std::array<Player, player_count> players {};
    Results results {};
    GameState state {
        player_count, player_count, {}, discarded_cards, deck, results.events, std::minstd_rand {static_cast<uint32_t>(rand() ^ 0x55555555)}
    };

    uint8_t next_rank = 1;

    auto kill_player = [&](std::size_t index) {
        if (!silent)
            std::cout << "Somebody is going to die now " << index << '\n';

        ASSERT(players[index].alive);
        players[index].alive = false;
        results.final_rank[index] = next_rank++;
        --state.players_alive;
        discarded_cards.take_cards(players[index].hand);
        players[index].engine.reset();
        ASSERT(players[index].hand.total_cards() == 0);
    };

    for (auto i = 0; i < player_count; ++i) {
        auto& initial_hand = data.hands[i];
        players[i].hand = initial_hand;
        state.hands[i] = &players[i].hand;

        if (players[i].hand.total_cards() == 0) {
            --state.players_alive;
            players[i].alive = false;
            results.instadied[i] = true;
            results.final_rank[i] = next_rank++;

            if (!silent)
                std::cout << "Player " << i << " died from initial double five\n";
            continue;
        }
        players[i].engine = VijfPlayer::from_command(player_commands[i]);
        if (!players[i].engine) {
            results.type = Results::Type::PlayerMisbehaved;
            results.player = i;
            return results;
        }
    }

    int turn = player_count - 1;
    results.moves_made.reserve(16);

    auto advance_to_next_player = [&] {
        ASSERT(state.players_alive > 0);
        do {
            if (turn == 0) {
                ASSERT(state.round_number < 100);
                ++state.round_number;
            }

            turn = (turn + 1) % player_count;
            ASSERT(turn < player_count);
        } while (!players[turn].alive);
    };

    while (state.players_alive >= 2) {
        ASSERT(state.players_alive <= deck.five_count() + 1);
        advance_to_next_player();

        auto& current_player = players[turn];
        ASSERT(current_player.engine);


        if (current_player.hand.get_max_of_card() == 4)
            add_event(results.events[turn], EventType::AllOfNonSpecial);

        if (current_player.hand.total_cards() > 1 && current_player.hand.card_types_count() == 1)
            add_event(results.events[turn], EventType::NoChoiceInCard);

        auto played = current_player.engine->take_turn(state, turn);

        if (!current_player.hand.has_card(played)) {
            // Player misbehaved stop game
            std::cout << "Player " << turn << " misbehaved! Played " << card_to_char_repr(played) << " while hand: " << current_player.hand.to_string_repr() << '\n';
            results.type = Results::Type::PlayerMisbehaved;
            results.player = turn;
            return results;
        }

        if (!silent)
            std::cout << "Player " << turn << " played " << card_to_char_repr(played) << " from " << current_player.hand.to_string_repr() << '\n';
        current_player.hand.play_card(played);
        CardNumber lowest = CardNumber::King;
        if (current_player.hand.total_cards() > 0)
            lowest = current_player.hand.get_lowest_card();

        results.moves_made.push_back(played);

        if (played == CardNumber::RuleCard) {
            if (!silent)
                std::cout << "Rule card played\n";

            int kills = 0;
            int innerTurn = (turn + 1) % player_count;
            while (innerTurn != turn) {
                if (!silent)
                    std::cout << "Inner round on " << innerTurn << '\n';

                if (players[innerTurn].alive) {
                    for (auto card_from_rules_card = 0; card_from_rules_card < 2; ++card_from_rules_card) {
                        auto card = deck.take_card();
                        if (!silent)
                            std::cout << "Got card " << card_to_char_repr(card) << '\n';

                        players[innerTurn].hand.add_card(card);
                        if (card == CardNumber::Five) {
                            ++kills;
                            if (!silent)
                                std::cout << "Player " << innerTurn << " died because of 5 from rules played by " << turn << '\n';
                            kill_player(innerTurn);
                            break;
                        }
                    }
                }
                innerTurn = (innerTurn + 1) % player_count;
            }

            if (kills == 4) {
                add_event(results.events[turn], EventType::RulesQuadrupleKill);
            } else if (kills == 3) {
                add_event(results.events[turn], EventType::RulesTripleKill);
            } else if (kills == 2) {
                add_event(results.events[turn], EventType::RulesDoubleKill);
            }
        } else {
            auto cards_to_get = number_of_card_to_get(played);
            if (!silent)
                std::cout << "Which means getting: " << cards_to_get << " cards\n";
            while (cards_to_get > 0) {
                CardNumber new_card = deck.take_card();
                current_player.hand.add_card(new_card);
                if (new_card == CardNumber::Five) {
                    if (!silent)
                        std::cout << "Died because of got vijf\n";

                    if ((number_of_card_to_get(played) - number_of_card_to_get(lowest)) > cards_to_get)
                        add_event(results.events[turn], EventType::CouldHaveSavedYourSelf);

                    if (current_player.hand.has_card(CardNumber::RuleCard))
                        add_event(results.events[turn], EventType::DiedWithRuleCard);

                    kill_player(turn);


                    break;
                }
                if (new_card == CardNumber::RuleCard)
                    break;
                --cards_to_get;
            }
        }

        if (state.players_alive > 1 && current_player.alive && current_player.hand.total_cards() == 0) {
            if (!silent)
                std::cout << "No cards left player " << turn << " has to stop\n";
            kill_player(turn);
            add_event(results.events[turn], EventType::RanOutOfCards);
        }
    }

    ASSERT(state.players_alive == 1);
    ASSERT(std::count_if(players.begin(), players.end(), [](Player const& player) { return player.alive; }) == 1);

    for (auto i = 0u; i < players.size(); ++i) {
        if (players[i].alive) {
            results.player = i;
            ASSERT(next_rank == 5);
            results.final_rank[i] = next_rank;
            if (players[i].hand.total_cards() == 0)
                add_event(results.events[turn], EventType::WonWithNoCards);
            break;
        }
    }
    results.rounds_played = state.round_number;

    return results;
}

template<typename EngineType>
StartData generate_random_start(EngineType& rng)
{
    constexpr int initial_hand_size = 3;

    StartData data;

    data.deck = OrderedCardStack::from_card_stack(CardStack::default_deck(true, 2));
    data.deck.shuffle(rng);

    auto& deck = data.deck;

    std::size_t fives_left_over = 0;

    for (int i = 0; i < player_count; ++i) {
        CardStack& hand = data.hands[i];
        for (int j = 0; j < initial_hand_size; ++j) {
            CardNumber card = deck.take_card();
            hand.add_card(card);
        }

        auto fives_in_hand = hand.remove_fives();
        if (fives_in_hand == 0)
            continue;

        fives_left_over += fives_in_hand;
        while (fives_in_hand > 0) {
            CardNumber card = deck.take_card();

            hand.add_card(card);
            if (card == CardNumber::Five) {
                data.discarded.take_cards(hand);
                ASSERT(hand.total_cards() == 0);
                ASSERT(fives_in_hand > 0);
                break;
            }
            --fives_in_hand;
        }

        if (fives_in_hand == 0) {
            ASSERT(!hand.has_card(CardNumber::Five));
            ASSERT(hand.total_cards() == initial_hand_size);
        }
    }

    if (fives_left_over > 0) {
        // Don't shuffle if we don't have to
        deck.add_fives(fives_left_over);
        deck.shuffle(rng);
    }

    ASSERT((std::size_t)std::count_if(data.hands.begin(), data.hands.end(), [](CardStack& hand) { return hand.total_cards() > 0; }) == 1 + deck.five_count());

    return data;
}

template StartData generate_random_start<std::mt19937>(std::mt19937& rng);


template StartData generate_random_start<std::default_random_engine>(std::default_random_engine& rng);


}

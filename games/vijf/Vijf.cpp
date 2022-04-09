#include <memory>
#include <iostream>
#include "Vijf.h"
#include "players/VijfPlayer.h"
#include "players/ProcessPlayer.h"
#include "players/BasicPlayers.h"

namespace Vijf {

bool silent = true;

Results play_game(StartData data)
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
    GameState state {
        player_count, player_count, {}, discarded_cards, deck
    };

    auto kill_player = [&](std::size_t index) {
        if (!silent)
            std::cout << "Somebody is going to die now " << index << '\n';

        ASSERT(players[index].alive);
        players[index].alive = false;
        --state.players_alive;
        discarded_cards.take_cards(players[index].hand);
        players[index].engine.reset();
        ASSERT(players[index].hand.total_cards() == 0);
    };

    Results results {};

    for (auto i = 0; i < player_count; ++i) {
        auto& initial_hand = data.hands[i];
        players[i].hand = initial_hand;
        state.hands[i] = &players[i].hand;

        if (players[i].hand.total_cards() == 0) {
            --state.players_alive;
            players[i].alive = false;
            results.instadied[i] = true;

            if (!silent)
                std::cout << "Player " << i << " died from initial double five\n";
            continue;
        }
        players[i].engine = std::make_unique<RandomPlayer>();

//        players[i].engine = std::make_unique<ProcessPlayer>(std::vector<std::string>{"cmake-build-release/VijfBot"});
//        players[i].engine = std::make_unique<ProcessPlayer>(std::vector<std::string> { "java", "JaVijf" });
        //        players[i].engine = std::make_unique<ProcessPlayer>(std::vector<std::string>{
        //            "podman", "run", "--network=none", "--cpus", "0.5", "--memory=128m", "--cap-drop=all", "--rm", "--interactive",
        //            "cpp-example"
        //            //                "python-example"
        //            //              "java-example"
        //            //                "zig-example"
        //        });
        continue;

        //        players[i].engine = std::make_unique<HighestFirst>();

//        if (i == 4) {
//            players[i].engine = std::make_unique<ProcessPlayer>(std::vector<std::string> {
//                "podman", "run", "--network=none", "--cpus", "0.5", "--memory=128m", "--cap-drop=all", "--rm", "--interactive",
//                //                "cpp-example"
//                "python-example"
//                //              "java-example"
//                //                "zig-example"
//            });
//
//        } else if (i == 1) {
//            players[i].engine = std::make_unique<ProcessPlayer>(std::vector<std::string> {
//                "podman", "run", "--network=none", "--cpus", "0.5", "--memory=128m", "--cap-drop=all", "--rm", "--interactive",
//                "cpp-example"
//                //                "python-example"
//                //              "java-example"
//                //                "zig-example"
//            });
//
//        } else if (i == 0) {
//            //            players[i].engine = std::make_unique<ProcessPlayer>(std::vector<std::string>{"cmake-build-release/VijfBot"});
//            //            players[i].engine = std::make_unique<ProcessPlayer>(std::vector<std::string>{"python3", "examples/run.py"});
//            players[i].engine = std::make_unique<ProcessPlayer>(std::vector<std::string> {
//                "podman", "run", "--network=none", "--cpus", "0.5", "--memory=128m", "--cap-drop=all", "--rm", "--interactive",
//                //                "cpp-example"
//                //                "python-example"
//                "java-example"
//                //                "zig-example"
//            });
//            //            players[i].engine = std::make_unique<ProcessPlayer>(std::vector<std::string>{"java", "JaVijf"});
//            //            players[i].engine = std::make_unique<ProcessPlayer>(std::vector<std::string>{"javijf.exe"});
//            //            players[i].engine = std::make_unique<CheatingPlayer>();
//            //            players[i].engine = std::make_unique<RandomPlayer>();
//        } else {
//            players[i].engine = std::make_unique<RandomPlayer>();
//            //            players[i].engine = std::make_unique<LowestFirst>();
//            //            players[i].engine = std::make_unique<CheatingPlayer>();
//        }
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

        results.moves_made.push_back(played);

        if (played == CardNumber::RuleCard) {
            if (!silent)
                std::cout << "Rule card played\n";

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
                            if (!silent)
                                std::cout << "Player " << innerTurn << " died because of 5 from rules played by " << turn << '\n';
                            kill_player(innerTurn);
                            break;
                        }
                    }
                }
                innerTurn = (innerTurn + 1) % player_count;
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
        }
    }

    ASSERT(state.players_alive == 1);

    for (auto i = 0u; i < players.size(); ++i) {
        if (players[i].alive) {
            results.player = i;
            break;
        }
    }
    results.rounds_played = state.round_number;
    return results;
}

StartData generate_random_start(std::default_random_engine& rng)
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


}

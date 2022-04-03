#include <iostream>
#include <vector>
#include <array>
#include <random>
#include <algorithm>
#include <numeric>
#include <ctime>
#include <map>
#include <assert.h>
#include <stdint.h>
#include <string>
#include <sstream>
#include <fstream>
#include <thread>
#include "Process.h"


static bool silent = true;

#ifdef _MSC_VER
#define ASSERT_NOT_REACHED() __assume(false)
#elif defined(__GNUC__)
#define ASSERT_NOT_REACHED() __builtin_unreachable()
#else
#error Unknown compiler
#endif

enum class CardNumber {
    RuleCard,
    Joker,
    Ace,
    King,
    Queen,
    Jack,
    Ten,
    Nine,
    Eight,
    Seven,
    Six,
    Five,
    Four,
    Three,
    Two,
};

struct CardWithChar {
    CardNumber card;
    char char_representation;
};

std::array<CardWithChar, 15> all_cards
        {{
                 {CardNumber::RuleCard, '+'},
                 {CardNumber::Joker, '*'},
                 {CardNumber::Ace, 'A'},
                 {CardNumber::King, 'K'},
                 {CardNumber::Queen, 'Q'},
                 {CardNumber::Jack, 'J'},
                 {CardNumber::Ten, 'T'},
                 {CardNumber::Nine, '9'},
                 {CardNumber::Eight, '8'},
                 {CardNumber::Seven, '7'},
                 {CardNumber::Six, '6'},
                 {CardNumber::Five, '5'},
                 {CardNumber::Four, '4'},
                 {CardNumber::Three, '3'},
                 {CardNumber::Two, '2'},
         }};

using CardNumberUnderlying = std::underlying_type_t<CardNumber>;

static constexpr CardNumberUnderlying card_to_underlying(CardNumber number) {
    return static_cast<CardNumberUnderlying>(number);
}

char card_to_char_repr(CardNumber card) {
    return all_cards[card_to_underlying(card)].char_representation;
}

class CardStack {
public:

    [[nodiscard]] bool has_card(CardNumber card) const {
        return m_counts[card_to_underlying(card)] > 0;
    }

    [[nodiscard]] size_t card_count(CardNumber card) const {
        assert(card <= CardNumber::Two);
        return m_counts[card_to_underlying(card)];
    }

    [[nodiscard]] size_t total_cards() const {
        return std::accumulate(m_counts.begin(), m_counts.end(), 0u);
    }

    void play_card(CardNumber card) {
        assert(m_counts[card_to_underlying(card)] > 0);
        m_counts[card_to_underlying(card)]--;
    }

    size_t remove_fives() {
        return std::exchange(m_counts[card_to_underlying(CardNumber::Five)], 0);
    }

    void add_card(CardNumber card, size_t count = 1) {
        m_counts[card_to_underlying(card)] += count;
        assert(m_counts[card_to_underlying(card)] != 0);
    }

    void take_cards(CardStack& otherStack) {
        for (size_t i = 0; i < m_counts.size(); ++i)
            m_counts[i] += otherStack.m_counts[i];

        otherStack.m_counts.fill(0);
    }

    [[nodiscard]] std::string to_string_repr() const {
        std::ostringstream result;
        to_sstream(result);
        return std::move(*result.rdbuf()).str();
    }

    static CardStack default_deck(bool withRulesCard, size_t joker_count) {
        CardStack stack{};
        stack.m_counts.fill(4);
        stack.m_counts[card_to_underlying(CardNumber::RuleCard)] = withRulesCard ? 1 : 0;
        stack.m_counts[card_to_underlying(CardNumber::Joker)] = joker_count;
        return stack;
    }

    void to_sstream(std::ostringstream& ostringstream) const {
        const size_t cards = total_cards();

        ostringstream << cards;
        if (!cards)
            return;

        ostringstream << ' ';

        for (auto& [card, character] : all_cards) {
            if (auto amount = m_counts[card_to_underlying(card)]; amount > 0) {
                for (auto i = 0u; i < amount; ++i)
                    ostringstream << character;
            }
        }
    }

private:
    std::array<size_t, all_cards.size()> m_counts{};
};

class OrderedCardStack {
public:
    [[nodiscard]] size_t cards_left() const {
        return m_cards.size();
    }

    void shuffle(std::default_random_engine &engine) {
        std::shuffle(m_cards.begin(), m_cards.end(), engine);
    }

    [[nodiscard]] CardNumber take_card() {
        assert(!m_cards.empty());
        auto card = m_cards.back();
        m_cards.pop_back();
        return card;
    }

    [[nodiscard]] CardStack to_card_stack() const {
        CardStack stack{};
        for (auto& card : m_cards) {
            stack.add_card(card, 1);
        }
        return stack;
    }

    [[nodiscard]] size_t five_count() const {
        return std::count(m_cards.begin(), m_cards.end(), CardNumber::Five);
    }

    [[nodiscard]] std::string to_string_repr() const {
        return to_card_stack().to_string_repr();
    }

    void to_sstream_ordered(std::ostringstream& output) const {
      const size_t cards = m_cards.size();

      output << cards;
      if (!cards)
        return;

      output << ' ';

      for (auto i = m_cards.size(); i > 0; --i) {
        output << card_to_char_repr(m_cards[i - 1]);
      }
    }

    static OrderedCardStack from_card_stack(CardStack const &stack) {
        OrderedCardStack ordered_stack{};
        ordered_stack.m_cards.reserve(stack.total_cards());
        for (auto& [card, _] : all_cards) {
            auto count = stack.card_count(card);
            for (auto i = 0u; i < count; ++i) {
                ordered_stack.m_cards.emplace_back(card);
            }
        }
        return ordered_stack;
    }

    void add_fives(size_t count) {
        while (count) {
            m_cards.emplace_back(CardNumber::Five);
            --count;
        }
    }

    [[nodiscard]] std::optional<size_t> next_five() const {
        assert(five_count() > 0);
        for (auto i = m_cards.size(); i > 0; --i) {
            if (m_cards[i - 1] == CardNumber::Five) {
                return m_cards.size() - i;
            }
        }
        return {};
    }

private:
    std::vector<CardNumber> m_cards{};

};

size_t number_of_card_to_get(CardNumber number) {
    switch (number) {
        case CardNumber::RuleCard:
            [[fallthrough]];
        case CardNumber::Joker:
            return 0;
        case CardNumber::Ace:
            return 1;
        case CardNumber::King:
            return 13;
        case CardNumber::Queen:
            return 12;
        case CardNumber::Jack:
            return 11;
        case CardNumber::Ten:
            return 10;
        case CardNumber::Nine:
            return 9;
        case CardNumber::Eight:
            return 8;
        case CardNumber::Seven:
            return 7;
        case CardNumber::Six:
            return 6;
        case CardNumber::Five:
            return 5;
        case CardNumber::Four:
            return 4;
        case CardNumber::Three:
            return 3;
        case CardNumber::Two:
            return 2;
    }
    return 0;
}

constexpr int player_count = 5;

struct Results {
    enum class Type {
        PlayerWon,
        PlayerMisbehaved,
    };
    Type type { Type::PlayerWon };
    size_t player;
    int rounds_played = 0;
    std::array<bool, player_count> instadied{false, false, false, false, false};
    std::vector<CardNumber> moves_made;
};

struct GameState {
    size_t players_alive;
    size_t round_number;
    std::array<CardStack const*, player_count> hands;
    CardStack const& discarded_cards;
    OrderedCardStack const& deck; // No cheating!
};

class VijfPlayer {
public:
    virtual CardNumber take_turn(GameState const& game_state, size_t your_position) = 0;
    virtual ~VijfPlayer() = default;
};

static constexpr std::array<CardNumber, 15> low_to_high_cards = {
        CardNumber::RuleCard,
        CardNumber::Joker,
        CardNumber::Ace,
        CardNumber::Two,
        CardNumber::Three,
        CardNumber::Four,
        CardNumber::Five,
        CardNumber::Six,
        CardNumber::Seven,
        CardNumber::Eight,
        CardNumber::Nine,
        CardNumber::Ten,
        CardNumber::Jack,
        CardNumber::Queen,
        CardNumber::King,
};

class LowestFirst : public VijfPlayer {
public:
    CardNumber take_turn(const GameState &game_state, size_t your_position) override {
        auto& hand = *game_state.hands[your_position];
        assert(hand.total_cards() > 0);
        for (auto& card : low_to_high_cards) {
            if (hand.has_card(card))
                return card;
        }
        ASSERT_NOT_REACHED();
    }
};

class HighestFirst : public VijfPlayer {
public:
    CardNumber take_turn(const GameState &game_state, size_t your_position) override {
        auto& hand = *game_state.hands[your_position];
        for (auto it = low_to_high_cards.rbegin(); it != low_to_high_cards.rend(); ++it) {
            if (hand.has_card(*it))
                return *it;
        }
        ASSERT_NOT_REACHED();
    }
};



CardNumber random_pick(CardStack const& stack) {
    static std::minstd_rand engine {static_cast<uint32_t>(rand())};
    std::uniform_int_distribution<size_t> dist{0, stack.total_cards() - 1};
    auto index = dist(engine);
    for (auto const& card : low_to_high_cards) {
        auto count = stack.card_count(card);
        if (count > index)
            return card;
        index -= count;
        assert(card != CardNumber::King);
    }
    ASSERT_NOT_REACHED();
}

class RandomPlayer : public VijfPlayer {
public:
    RandomPlayer() = default;

    CardNumber take_turn(const GameState &game_state, size_t your_position) override {
        auto& hand = *game_state.hands[your_position];
        return random_pick(hand);
    }

};

class CheatingPlayer : public VijfPlayer {
public:
    CardNumber take_turn(const GameState &game_state, size_t your_position) override {
        auto next_five = game_state.deck.next_five();
        assert(next_five.has_value());
        assert(next_five < game_state.deck.cards_left());
        auto& hand = *game_state.hands[your_position];
        if (hand.has_card(CardNumber::RuleCard)) {
            if (next_five < game_state.players_alive * 2)
                return CardNumber::RuleCard;
        }


        bool any = false;
        CardNumber highest;
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
};

class ProcessPlayer : public VijfPlayer {
public:
    ProcessPlayer(std::vector<std::string> command) {
        assert(command.size() > 0);
        m_command = command;
        auto proc_or_fail = util::SubProcess::create(move(command));
        if (!proc_or_fail)
            return;

        auto& temp_process = *proc_or_fail;
        size_t timeTaken = 0;
        auto ready_response = temp_process.sendAndWaitForResponse("game 0 vijf\n", 1500, &timeTaken);
        if (!ready_response.has_value()) {
            std::cout << "Started to slow or did not output anything in response to: 'game 0 vijf' for " << m_command[0] << ' ' << m_command[m_command.size() - 1] << '\n';
            return;
        }
//        if (timeTaken > 100)
//            std::cout << "WARN: Slow startup took " << timeTaken << " ms for " << m_command[0] << ' ' << m_command[m_command.size() - 1] << '\n';

        if (*ready_response != "ready\n") {
            std::cout << "Did not get correct ready response, got _" << *ready_response << "_ from " << m_command[0] << ' ' << m_command[m_command.size() - 1] << '\n';
            return;
        }

        m_process = std::move(proc_or_fail);
    }

    CardNumber take_turn(const GameState &game_state, size_t your_position) override {
//        std::cout << "Process player taking turn now!\n";
        if (m_process) {
            auto result_or_failed = play_turn(game_state, your_position);
            if (result_or_failed.has_value())
                return result_or_failed.value();
            m_process->writeToWithTimeout("died\n", 10);
            m_process.reset();

            std::cerr << "Process player falling back on random player!\n";
        }
        auto& hand = *game_state.hands[your_position];
        return random_pick(hand);
    }

    ~ProcessPlayer() {
        if (m_process)
            m_process->writeToWithTimeout("died\n", 10);

    }

private:
    std::optional<CardNumber> play_turn(const GameState& game_state, size_t your_position) {
        assert(m_process);

        std::ostringstream message{};
        message << "turn " << (player_count + 2) << ' ' << your_position << ' ' << game_state.players_alive << ' ' << game_state.round_number << '\n';
        for (auto const& hand : game_state.hands) {
            hand->to_sstream(message);
            message << '\n';
        }

        game_state.discarded_cards.to_sstream(message);
        message << '\n';

        game_state.deck.to_card_stack().to_sstream(message);
        message << '\n';

        size_t timeTaken = 0;
        auto val = std::move(*message.rdbuf()).str();
        auto result = m_process->sendAndWaitForResponse(val, 100, &timeTaken);
        if (!result.has_value()) {
            std::cerr << "ProcessPlayer '" << m_command[0] << ' ' << m_command[m_command.size() - 1] << "' sent no response!\n";
            return std::nullopt;
        }

//        if (timeTaken > 5)
//            std::cout << "Slow response took " << timeTaken << " ms for " << m_command[0] << ' ' << m_command[m_command.size() - 1] << '\n';

        std::string_view view = *result;
        assert(view[view.length() - 1] == '\n');
        view.remove_suffix(1);

        if (view.find("play ") != 0 || view.size() < 6) {
            std::cout << "Player response from '" << m_command[0] << ' ' << m_command[m_command.size() - 1] << "'does not start with 'play ' or there is nothing after the space. Got _" << view << "_\n";
            std::cout << "When given: " << val;
            return std::nullopt;
        }

        char card_character = view[5];
        for (auto const& card : all_cards) {
            if (card.char_representation == card_character) {
//                std::cerr << "Decoded process player card '" << card_character << "'\n";
                return card.card;
            }
        }

        std::cout << "Player response does not map to a card got '" << card_character << "' which is not known.\n";
        return std::nullopt;
    }

    std::unique_ptr<util::SubProcess> m_process {nullptr};
    std::vector<std::string> m_command;
};


struct StartData {
  CardStack discarded;
  OrderedCardStack deck;
  std::array<CardStack, player_count> hands;


  [[nodiscard]] std::string to_string() const {
      std::ostringstream result;
      for (auto const& hand : hands) {
        hand.to_sstream(result);
        result << ' ';
      }

      deck.to_sstream_ordered(result);
      result << ' ';

      discarded.to_sstream(result);

      return std::move(*result.rdbuf()).str();
  }

};

StartData generate_random_start(std::default_random_engine &rng) {
  constexpr int initial_hand_size = 3;

  StartData data;

  data.deck = OrderedCardStack::from_card_stack(CardStack::default_deck(true, 2));
  data.deck.shuffle(rng);

  auto& deck = data.deck;

  size_t fives_left_over = 0;

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
        assert(hand.total_cards() == 0);
        assert(fives_in_hand > 0);
        break;
      }
      --fives_in_hand;
    }

    if (fives_in_hand == 0) {
      assert(!hand.has_card(CardNumber::Five));
      assert(hand.total_cards() == initial_hand_size);
    }
  }

  if (fives_left_over > 0) {
    // Don't shuffle if we don't have to
    deck.add_fives(fives_left_over);
    deck.shuffle(rng);
  }

  assert((size_t)std::count_if(data.hands.begin(), data.hands.end(), [](CardStack& hand) { return hand.total_cards() > 0; }) == 1 + deck.five_count());

  return data;
}

Results play_game(StartData data) {

    CardStack discarded_cards = data.discarded;
    OrderedCardStack deck {std::move(data.deck)};
    assert(deck.five_count() > 0);

    struct Player {
        CardStack hand {};
        bool alive { true };
        std::unique_ptr<VijfPlayer> engine {nullptr};
    };

    std::array<Player, player_count> players{};
    GameState state {
        player_count, player_count, {}, discarded_cards, deck
    };

    auto kill_player = [&](size_t index) {
        if (!silent)
            std::cout << "Somebody is going to die now " << index << '\n';

        assert(players[index].alive);
        players[index].alive = false;
        --state.players_alive;
        discarded_cards.take_cards(players[index].hand);
        players[index].engine.reset();
        assert(players[index].hand.total_cards() == 0);
    };

    Results results{};

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



//        players[i].engine = std::make_unique<ProcessPlayer>(std::vector<std::string>{"cmake-build-release/VijfBot"});
        players[i].engine = std::make_unique<ProcessPlayer>(std::vector<std::string>{"java", "JaVijf"});
//        players[i].engine = std::make_unique<ProcessPlayer>(std::vector<std::string>{
//            "podman", "run", "--network=none", "--cpus", "0.5", "--memory=128m", "--cap-drop=all", "--rm", "--interactive",
//            "cpp-example"
//            //                "python-example"
//            //              "java-example"
//            //                "zig-example"
//        });
        continue;


//        players[i].engine = std::make_unique<HighestFirst>();

        if (i == 4) {
          players[i].engine = std::make_unique<ProcessPlayer>(std::vector<std::string>{
              "podman", "run", "--network=none", "--cpus", "0.5", "--memory=128m", "--cap-drop=all", "--rm", "--interactive",
              //                "cpp-example"
                              "python-example"
//              "java-example"
              //                "zig-example"
          });

        } else if (i == 1) {
          players[i].engine = std::make_unique<ProcessPlayer>(std::vector<std::string>{
              "podman", "run", "--network=none", "--cpus", "0.5", "--memory=128m", "--cap-drop=all", "--rm", "--interactive",
                              "cpp-example"
              //                "python-example"
//              "java-example"
              //                "zig-example"
          });

        } else if (i == 0) {
//            players[i].engine = std::make_unique<ProcessPlayer>(std::vector<std::string>{"cmake-build-release/VijfBot"});
//            players[i].engine = std::make_unique<ProcessPlayer>(std::vector<std::string>{"python3", "examples/run.py"});
            players[i].engine = std::make_unique<ProcessPlayer>(std::vector<std::string>{
                "podman", "run", "--network=none", "--cpus", "0.5", "--memory=128m", "--cap-drop=all", "--rm", "--interactive",
//                "cpp-example"
//                "python-example"
                "java-example"
//                "zig-example"
            });
//            players[i].engine = std::make_unique<ProcessPlayer>(std::vector<std::string>{"java", "JaVijf"});
//            players[i].engine = std::make_unique<ProcessPlayer>(std::vector<std::string>{"javijf.exe"});
//            players[i].engine = std::make_unique<CheatingPlayer>();
//            players[i].engine = std::make_unique<RandomPlayer>();
        } else {
            players[i].engine = std::make_unique<RandomPlayer>();
//            players[i].engine = std::make_unique<LowestFirst>();
//            players[i].engine = std::make_unique<CheatingPlayer>();
        }
    }

    int turn = player_count - 1;
    results.moves_made.reserve(16);

    auto advance_to_next_player = [&] {
        assert(state.players_alive > 0);
        do {
            if (turn == 0) {
                assert(state.round_number < 100);
                ++state.round_number;
            }

            turn = (turn + 1) % player_count;
            assert(turn < player_count);
        } while(!players[turn].alive);
    };

    while (state.players_alive >= 2) {
        assert(state.players_alive <= deck.five_count() + 1);
        advance_to_next_player();


        auto& current_player = players[turn];
        assert(current_player.engine);

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

    assert(state.players_alive == 1);

    for (auto i = 0u; i < players.size(); ++i) {
        if (players[i].alive) {
            results.player = i;
            break;
        }
    }
    results.rounds_played = state.round_number;
    return results;
}

int main() {
    // FIXME: Better random seeding?
    srand(time(nullptr));

    std::vector<std::thread> threads;

//    for (int tid = 0; tid < 10; tid++) {
//      threads.emplace_back([tid] {
    uint32_t seed = rand();
    //    uint32_t seed = 343173667;

    std::cout << "Seed: " << seed << '\n';
    auto engine = std::default_random_engine{seed};

    //    play_game(engine);
    //    return 0;

    std::array<size_t, 5> won_games{};
    std::array<size_t, 5> instadied{};
    std::array<size_t, 26> rounds{};
    std::array<size_t, 52> moves{};

  //        std::ofstream games{"games_played.txt"};

    for (auto i = 0; i < 100; ++i) {
        auto initial_data = generate_random_start(engine);
        //          std::string start_string = initial_data.to_string();
        auto results = play_game(std::move(initial_data));
        if (results.type == Results::Type::PlayerMisbehaved) {
            std::cout << "Misbehaving by " << results.player << '\n';
            break;
        }
        if (results.player >= player_count) {
            std::cout << "Failed? with seed: " << seed << " , " << i << "=> "
                      << results.player << '\n';
            continue;
        }

        for (int j = 0; j < 5; ++j) {
            if (results.instadied[j])
                ++instadied[j];
        }
        ++rounds[results.rounds_played];
        ++won_games[results.player];

        assert(!results.moves_made.empty());
        ++moves[results.moves_made.size()];

        //          std::ostringstream moves_string;
        //          moves_string << results.moves_made.size() << ' ';
        //
        //          for (auto &card : results.moves_made)
        //            moves_string << card_to_char_repr(card);
        //
        ////          games << start_string << "; " << moves_string.str() << '\n';
    }

    //        games.close();

    for (auto i = 0; i < 5; ++i) {
        std::cout << i << " won " << won_games[i]
                  << " times and died instant " << instadied[i] << " times\n";
    }

    for (auto i = 0u; i < rounds.size(); ++i) {
        if (rounds[i] > 0 || i < 5)
            std::cout << rounds[i] << " game finished in round " << i << '\n';
    }

    for (auto i = 0u; i < moves.size(); ++i) {
        if (moves[i] > 0 || i < 10)
            std::cout << moves[i] << " game finished in " << i << " moves\n";
    }

//    std::cout << "Thread " << tid << " Done!\n";
//  });
//    }

//    for (auto& t : threads)
//        t.join();

    return 0;
}

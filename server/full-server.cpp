#include "crow/json.h"
#include <algorithm>
#include <atomic>
#include <cstdint>
#include <iterator>
#include <string_view>
#include <thread>
#include <tuple>
#include <utility>
#include <variant>
#include <optional>
#define CROW_DISABLE_STATIC_DIR
#include "crow.h"
#include <chrono>
#include <mutex>
#include "../util/Deferred.h"
#include "../util/Assertions.h"
#include <pqxx/transaction>
#include <pqxx/result>
#include <asio.hpp>
#include <charconv>
#include <memory>

void post_close(crow::websocket::connection& connection, std::string const& message = "quit") {
    auto* ptr = dynamic_cast<crow::websocket::Connection<crow::SocketAdaptor, crow::SimpleApp>*>(&connection);
    ASSERT(ptr);
    ptr->dispatch([&]{
        connection.close(message);
    });
}



class GuessPlayer {
public:
    enum class GuessResult {
        OtherPlayerFailed,
        Higher,
        Lower
    };

    static std::string result_to_string(GuessResult result) {
        switch (result) {
            case GuessResult::OtherPlayerFailed:
                return "incorrect";
            case GuessResult::Higher:
                return "higher";
            case GuessResult::Lower:
                return "lower";
        }

        return "?";
    }

    virtual void guess_made(int, GuessResult) {};
    virtual std::optional<int> guess() = 0;
    virtual ~GuessPlayer() {}
};

class IncrementingPlayer : public GuessPlayer {
public:
    explicit IncrementingPlayer(int start)
        : m_value(start) {}

    std::optional<int> guess() override {
        return m_value++;
    }

private:
    int m_value {};

};

class InteractivePlayer: public GuessPlayer {
public:
    InteractivePlayer(std::string& input, std::vector<std::string>& output) :
        m_input_buffer(input), m_output_buffer(output) {}


    void guess_made(int value, GuessPlayer::GuessResult result) override {
        std::ostringstream message {};
        message << "result " << value << ' ' << result_to_string(result);
        m_output_buffer.emplace_back(message.str());
    }

    std::optional<int> guess() override {
        if (!sent_output) {
            sent_output = true;
            m_output_buffer.push_back("guess");
        }

        std::cout << "Guessing for interactive: Have\n" << m_input_buffer << '\n';
        std::cout << "Current output buffer:\n" << m_output_buffer.size() << " messages\n";
        auto end_of_line = m_input_buffer.find('\n');
        if (end_of_line == std::string::npos)
            return std::nullopt;

        int value = -1;
        auto result = std::from_chars(m_input_buffer.data(), m_input_buffer.data() + end_of_line, value);
        if (result.ec != std::errc{})
            return std::nullopt; // INVALID!! STOP HERE!

        sent_output = false;
        m_input_buffer.erase(0, end_of_line + 1);

        return value;
    }

private:
    bool sent_output = false;
    std::string& m_input_buffer;
    std::vector<std::string>& m_output_buffer;
};

struct PlayerFactory {
    struct InteractivePlayerInput {
        std::string& input;
        std::vector<std::string>& output;
    };

    explicit PlayerFactory(std::string command_)
        : data(std::move(command_)) {}

    explicit PlayerFactory(std::string& input, std::vector<std::string>& output)
        : data(InteractivePlayerInput{input, output}) {}

    bool is_interactive() const {
        return !std::holds_alternative<std::string>(data);
    }

    bool contains_input(std::string const& other) const {
        ASSERT(is_interactive());
        return &(std::get<InteractivePlayerInput>(data).input) == &other;
    }

    template<typename PlayerType>
    std::unique_ptr<PlayerType> construct_interactive() const {
        ASSERT(is_interactive());
        auto& references = std::get<InteractivePlayerInput>(data);
        return std::make_unique<PlayerType>(references.input, references.output);
    }

    void override_interactive_player(std::string& input, std::vector<std::string>& output) {
        ASSERT(is_interactive());
        data.emplace<InteractivePlayerInput>(input, output);
    }

    std::string const& command() const {
        ASSERT(!is_interactive());
        return std::get<std::string>(data);
    }
private:
    std::variant<std::string, InteractivePlayerInput> data;
};

using PlayerIdentifier = uint32_t;

enum class InteractiveTickResult {
    Running,
    WaitingOnYou,
    DoneCleanUpState,
    DoneClearStateOnly,
    FailedCleanUpState,
    FailedClearStateOnly,
};

struct InteractiveGameState {
    bool game_in_progress() {
        return done.load() == DoneState::Running;
    }

    void set_done_if_in_progress() {
        DoneState expected { DoneState::Running };
        bool exchanged = done.compare_exchange_strong(expected, DoneState::Done);
        ASSERT(exchanged || expected == DoneState::Failed);
    }

    void mark_failed() {
        done.store(DoneState::Failed);
    }

    bool has_failed() {
        return done.load() == DoneState::Failed;
    }

    bool can_run_exclusive() {
        return !run_lock.test_and_set();
    }

    void done_running() {
        run_lock.clear();
    }

    bool mark_player_done_is_last() {
        auto old_value = active_interactive_players.fetch_sub(1);
        ASSERT(old_value >= 1);
        return old_value == 1;
    }

    virtual ~InteractiveGameState() = 0;

    // FIXME: Protect me with friend or something
    void set_num_interactive_players(uint32_t amount) {
        active_interactive_players.store(amount);
    }

private:
    enum class DoneState {
        Running,
        Done,
        Failed
    };

    std::atomic<uint32_t> active_interactive_players { 0 };
    static_assert(std::atomic<uint32_t>::is_always_lock_free);

    std::atomic<DoneState> done { DoneState::Running };
    static_assert(std::atomic<DoneState>::is_always_lock_free);

    std::atomic_flag run_lock = ATOMIC_FLAG_INIT;
};

InteractiveGameState::~InteractiveGameState() {}

struct GuessGameState : public InteractiveGameState {
   int number = -1;

   int turnForPlayer = 0;
   std::vector<std::unique_ptr<GuessPlayer>> players{};

   explicit GuessGameState(int value, std::vector<std::unique_ptr<GuessPlayer>> new_players)
    : number(value), players(std::move(new_players)) {
       std::cout << "Looking for" << number << '\n';
   }

   virtual ~GuessGameState() {
       std::cout << "Destroying game state!\n";
   }
};


struct InteractiveGame {
    virtual ~InteractiveGame() {}

    virtual uint32_t get_num_players() const = 0;
    virtual std::vector<std::string> const& available_algortihms() const = 0;
    virtual InteractiveGameState* start_game(std::vector<PlayerFactory> const& setup) const = 0;

    virtual std::optional<PlayerIdentifier> tick_interactive_game(InteractiveGameState* game_data) const = 0;
};

template<typename GameState>
class MultiplayerGame : public InteractiveGame {
    virtual InteractiveGameState* start_game(std::vector<PlayerFactory> const& setup) const final {
        return game_from_commands(setup);
    }

    virtual GameState* game_from_commands(std::vector<PlayerFactory> const& commands) const = 0;

    virtual std::optional<PlayerIdentifier> tick_interactive_game(InteractiveGameState* game_data) const final {
        ASSERT(dynamic_cast<GameState*>(game_data));
        return tick_game_state(*static_cast<GameState*>(game_data));
    }

    virtual std::optional<PlayerIdentifier> tick_game_state(GameState& state) const = 0;
};

struct GuessGame final : public MultiplayerGame<GuessGameState> {

    explicit GuessGame(uint32_t num_players)
        : m_num_players(num_players) {
            ASSERT(m_num_players > 0);
        }

    uint32_t get_num_players() const override {
        return m_num_players;
    }

    std::vector<std::string> const& available_algortihms() const override
    {
        return m_algorithms;
    }

    GuessGameState* game_from_commands(std::vector<PlayerFactory> const& setup) const override
    {
        int number = rand() % 1000;
        std::vector<std::unique_ptr<GuessPlayer>> players;
        for (auto& command : setup) {
            if (command.is_interactive()) {
                players.emplace_back(command.construct_interactive<InteractivePlayer>());
            } else {
                int start = 5 + (rand() % 20);
                players.emplace_back(std::make_unique<IncrementingPlayer>(number - start));
            }
        }

        return new GuessGameState {
            number,
            std::move(players)
        };
    }

    std::optional<PlayerIdentifier> tick_game_state(GuessGameState& game_state) const override
    {

        while (true) {
            PlayerIdentifier this_player = game_state.turnForPlayer;
            auto& player = game_state.players[game_state.turnForPlayer];
            auto potential_guess = player->guess();
            if (!potential_guess.has_value()) {
                std::cout << "Player " << this_player << " has no move ready waiting...\n";
                return this_player;
            }

            int guess = *potential_guess;

            if (guess == game_state.number) {
                std::cout << "Win for player " << this_player;
                break;
            }

            GuessPlayer::GuessResult result = guess < game_state.number ? GuessPlayer::GuessResult::Higher : GuessPlayer::GuessResult::Lower;
            player->guess_made(guess, result);

            for (size_t i = 0; i < game_state.players.size(); ++i) {
                if (i == this_player)
                    continue;
                game_state.players[i]->guess_made(guess, GuessPlayer::GuessResult::OtherPlayerFailed);
            }

            game_state.turnForPlayer = (game_state.turnForPlayer + 1) % game_state.players.size();
        }

        return {};
    };
private:
    uint32_t m_num_players{0};
    std::vector<std::string> m_algorithms {
        "incrementing",
        "random",
    };
};

class TTTPlayer {
public:
    enum class FieldValue {
        Empty,
        Cross,
        Circle
    };

    static char to_char(FieldValue value) {
        switch (value) {
            case FieldValue::Empty:
                return '-';
            case FieldValue::Cross:
                return 'X';
            case FieldValue::Circle:
                return 'O';
        }
        ASSERT(false);
    }

    virtual std::optional<size_t> play(std::array<FieldValue, 9> const& field, FieldValue you) = 0;
    virtual ~TTTPlayer() {}
};


class InteractiveTTTPlayer: public TTTPlayer {
public:
    InteractiveTTTPlayer(std::string& input, std::vector<std::string>& output) :
        m_input_buffer(input), m_output_buffer(output) {}

    std::optional<size_t> play(std::array<FieldValue, 9> const& field, FieldValue you) override {
        if (!sent_output) {
            sent_output = true;
            std::ostringstream message {};
            ASSERT(you == FieldValue::Circle || you == FieldValue::Cross);
            message << "turn " << to_char(you) << ' ';
            for (size_t i = 0; i < field.size(); ++i) {
                if (i > 0 && i % 3 == 0)
                    message << '|';
                message << to_char(field[i]);
            }
            m_output_buffer.emplace_back(message.str());
        }

        auto end_of_line = m_input_buffer.find('\n');
        if (end_of_line == std::string::npos)
            return std::nullopt;

        int value = -1;
        auto result = std::from_chars(m_input_buffer.data(), m_input_buffer.data() + end_of_line, value);
        if (result.ec != std::errc{})
            return std::nullopt; // INVALID!! STOP HERE!

        if (value < 0 || value > 8)
            return std::nullopt; // INVALID!! STOP HERE!

        if (field[value] != TTTPlayer::FieldValue::Empty)
            return std::nullopt; // INVALID!! STOP HERE!

        sent_output = false;
        m_input_buffer.erase(0, end_of_line + 1);

        return value;
    }

private:
    bool sent_output = false;
    std::string& m_input_buffer;
    std::vector<std::string>& m_output_buffer;
};

class FirstSpotPlayer : public TTTPlayer {
    std::optional<size_t> play(std::array<FieldValue, 9> const& field, FieldValue) override {
        for (size_t i = 0; i < field.size(); ++i) {
            if (field[i] == FieldValue::Empty)
                return i;
        }
        ASSERT(false);
    }
};


struct TTTGameState : public InteractiveGameState {
    using FieldValue = TTTPlayer::FieldValue;

    PlayerIdentifier turnForPlayer = 0;
    std::array<FieldValue, 2> player_mapping{};
    std::array<FieldValue, 9> field{};

    std::array<std::unique_ptr<TTTPlayer>, 2> players{};

    explicit TTTGameState(std::array<std::unique_ptr<TTTPlayer>, 2> new_players, FieldValue starting_player)
        : player_mapping({starting_player, invert(starting_player)}), players(std::move(new_players))
    {
        field.fill(FieldValue::Empty);
    }

    static FieldValue invert(FieldValue value) {
        switch(value) {
            case FieldValue::Circle:
                return FieldValue::Cross;
            case FieldValue::Cross:
                return FieldValue::Circle;
            case FieldValue::Empty:
                return FieldValue::Empty;
        }
        ASSERT(false);
    }

    bool win_for(FieldValue symbol) const {
        ASSERT(symbol == FieldValue::Cross || symbol == FieldValue::Circle);
        ASSERT(!triple_for(invert(symbol)));
        return triple_for(symbol);
    }
private:

    static std::array<std::tuple<uint8_t, uint8_t, uint8_t>, 8> constexpr lines = {{
        {0, 1, 2},
        {3, 4, 5},
        {6, 7, 8},

        {0, 3, 6},
        {1, 4, 7},
        {2, 5, 8},

        {0, 4, 8},
        {2, 4, 6}
    }};

    bool triple_for(FieldValue symbol) const {
        for (auto& [a, b, c] : lines) {
            if (field[a] == symbol && field[b] == symbol && field[c] == symbol)
                return true;
        }
        return false;
    }

};


struct TTTGame final : public MultiplayerGame<TTTGameState> {
    virtual uint32_t get_num_players() const override {
        return 2;
    };

    virtual std::vector<std::string> const& available_algortihms() const override {
        return m_algorithms;
    };


    TTTGameState* game_from_commands(std::vector<PlayerFactory> const& setup) const override
    {
        ASSERT(setup.size() == 2);
        std::array<std::unique_ptr<TTTPlayer>, 2> players;
        for (size_t i = 0; i < 2; ++i) {
            auto& command = setup[i];
            if (command.is_interactive()) {
                players[i] = command.construct_interactive<InteractiveTTTPlayer>();
            } else {
                players[i] = std::make_unique<FirstSpotPlayer>();
            }
        }

        return new TTTGameState {
            std::move(players), TTTGameState::FieldValue::Cross
        };
    }

    std::optional<PlayerIdentifier> tick_game_state(TTTGameState& game_state) const override
    {

        while (true) {
            PlayerIdentifier this_player = game_state.turnForPlayer;
            auto& player = game_state.players[game_state.turnForPlayer];
            TTTGameState::FieldValue player_symbol = game_state.player_mapping[this_player];
            auto potential_guess = player->play(game_state.field, player_symbol);
            if (!potential_guess.has_value()) {
                std::cout << "Player " << this_player << " has no move ready waiting...\n";
                return this_player;
            }

            size_t guess = *potential_guess;
            if (game_state.field[guess] != TTTPlayer::FieldValue::Empty) {
                // Player misbehaved!!! Somehow communicate this
                break;
            }
            game_state.field[guess] = player_symbol;

            if (game_state.win_for(player_symbol)) {
                break;
            }

            game_state.turnForPlayer = (game_state.turnForPlayer + 1) % game_state.players.size();
        }

        return {};
    };

private:

    std::vector<std::string> m_algorithms {
        "random",
        "first-spot",
    };

};

class ConnectionHolder {
public:
    ConnectionHolder(crow::websocket::connection& connection) : m_connection(connection) {}
private:
    friend struct ObserverState;
    friend struct Observers;

    crow::websocket::connection& m_connection;
    // If true, you can no longer use the connection provided,
    std::atomic_flag closed = ATOMIC_FLAG_INIT;
};

struct Observers {

    void extend_with(Observers& observers) {
        if (observers.m_connections.empty())
            return;

        m_connections.reserve(m_connections.size() + observers.m_connections.size());
        m_connections.insert(
            m_connections.end(),
            std::make_move_iterator(observers.m_connections.begin()),
            std::make_move_iterator(observers.m_connections.end())
        );
        observers.m_connections.clear();
    }

    void send_message(std::string const& message) const {
        std::cout << "Sending \"" << message << "\" to " << m_connections.size() << " observers!\n";
        std::cout.flush();
        for_each_connection([&message](auto& connection) {
            std::cout << "Sending to " << &connection << '\n';
            connection.send_text(message);
        });
        std::cout << "sending done!\n";
    }

    void close_all(std::string const& reason = "quit") {
        for_each_connection([&reason](auto& connection) {
            post_close(connection, reason);
        });
        m_connections.clear();
    }

    void add_observer(std::weak_ptr<ConnectionHolder> connection) {
        m_connections.emplace_back(connection);
    }

private:

    template<typename Func>
    void for_each_connection(Func func) const {
        for (auto& weak_connection : m_connections) {
            auto shared_ptr = weak_connection.lock();
            if (!shared_ptr) {
                std::cout << "Shared ptr no longer valid!\n";
                continue;
            }

            if (!shared_ptr->closed.test())
                func(shared_ptr->m_connection);
            else
                std::cout << "Observer closed!\n";
        }
    }

    std::vector<std::weak_ptr<ConnectionHolder>> m_connections;
};

struct InteractiveGameSetup {
    std::optional<PlayerIdentifier> player_id;
    InteractiveGame const* game = nullptr;
    InteractiveGameState* game_data = nullptr;
    Observers observers;

    std::string error_message{};

    bool has_error() const {
        return !error_message.empty();
    }

    bool ready_to_play() const {
        return game_data;
    }

    static InteractiveGameSetup invalid(std::string message) {
        return {
            0,
            nullptr,
            nullptr,
            {},
            std::move(message),
        };
    }

    static InteractiveGameSetup not_ready(std::optional<PlayerIdentifier> id = std::nullopt) {
        return {
            id, nullptr, nullptr, {}, ""
        };
    }
};

struct AvailableGame {
    std::string name;
    std::string baseName;
    std::unique_ptr<InteractiveGame> game;
};

std::vector<AvailableGame> games;

static std::string dummy_string;
static std::vector<std::string> dummy_vector;

struct SetupMatch {
    uint32_t interactive_players_left_to_give_state;
    std::string code{};
    std::vector<PlayerFactory> players{};
    InteractiveGame const* game = nullptr;
    InteractiveGameState* game_data = nullptr;
    Observers waiting_observers{};

    bool is_empty_spot(uint32_t i) const {
        return players[i].is_interactive() && players[i].contains_input(dummy_string);
    }

    uint32_t next_empty_player_slot() const {
        for (uint32_t i = 0; i < players.size(); ++i) {
            if (is_empty_spot(i))
                return i;
        }
        return players.size();
    }

    void insert_player(uint32_t index, std::string& input, std::vector<std::string>& output) {
        ASSERT(is_empty_spot(index));
        players[index].override_interactive_player(input, output);
        ASSERT(!is_empty_spot(index));
    }

    bool is_ready() const {
        for (uint32_t i = 0; i < players.size(); ++i) {
            if (players[i].is_interactive() && is_empty_spot(i))
                return false;
        }
        return true;
    }
};

std::uniform_int_distribution<char> letter_distribution('A', 'Z');
std::minstd_rand random_engine;

std::timed_mutex match_lock;
std::vector<SetupMatch> prepared_matches;

std::string random_match_code(std::string const& game)
{
    std::string match_code = game + "-XXX";
    for (size_t i = match_code.size() - 3; i < match_code.size(); ++i) {
        match_code[i] = letter_distribution(random_engine);
    }
    return match_code;
}

AvailableGame const* find_game(std::string_view game_name) {
    auto game_or_end = std::find_if(games.begin(), games.end(), [&](auto& game) {
        return game.name == game_name;
    });
    if (game_or_end == games.end())
        return nullptr;

    return game_or_end.base();
}

InteractiveGame const* get_game_ptr(std::string_view game_name) {
    auto* game = find_game(game_name);
    if (!game)
        return nullptr;
    return game->game.get();
}

struct PreparedMatch {
    bool ready{false};
    std::string code_or_error;

    static PreparedMatch fail(std::string error) {
        return {
            false,
            std::move(error)
        };
    }

    static PreparedMatch match_ready(std::string match_code) {
        return {
            true,
            std::move(match_code)
        };
    }
};

PreparedMatch prepare_match(std::string const& game_name, std::vector<std::string_view> commands) {
    auto* game_ptr = get_game_ptr(game_name);
    if (!game_ptr)
        return PreparedMatch::fail("Unknown game!");

    if (game_ptr->get_num_players() != commands.size())
        return PreparedMatch::fail("Invalid number of players!");
    ASSERT(commands.size() > 0);

    std::string match_code = random_match_code(game_name);

    std::vector<PlayerFactory> player_commands;
    uint32_t interactive_players = 0;

    for (auto& command : commands) {
        if (!command.empty()) {
            player_commands.emplace_back(std::string{command});
        } else {
            ++interactive_players;
            player_commands.emplace_back(dummy_string, dummy_vector);
        }
    }

    if (interactive_players == 0)
        return PreparedMatch::fail("Only bot players, at the moment not allowed!");

    {
        std::lock_guard _lock(match_lock);

        for (size_t i = 0; i < 25; ++i) {
            if (std::none_of(prepared_matches.begin(), prepared_matches.end(), [&](auto& val) {
                return val.code == match_code;
            })) {
                break;
            } else if (i == 24) {
                return PreparedMatch::fail("No space available to play game, try again later");
            }

            match_code = random_match_code(game_name);
        }

        prepared_matches.emplace_back(interactive_players, match_code, std::move(player_commands), game_ptr);
    }

    return PreparedMatch::match_ready(match_code);
}

InteractiveGameSetup setup_game(bool have_id, std::string const& match_code_full, std::string& input, std::vector<std::string>& output) {
    auto seperator = match_code_full.find(";");
    std::optional<size_t> preffered_id{};
    bool singleplayer = false;
    if (!have_id && seperator != std::string::npos) {
        if (seperator + 1 < match_code_full.size() && match_code_full[seperator + 1] == 'S') {
            ++seperator;
            singleplayer = true;
        } else if (seperator + 1 >= match_code_full.size()) {
            return InteractiveGameSetup::invalid("No specifier for preference");
        }

        std::cout << "Reading number from: " << match_code_full.substr(seperator + 1) << '\n';
        std::cout << "Single? " << singleplayer << " slash: " << seperator << " size " << match_code_full.size() << '\n';


        if (seperator + 1 < match_code_full.size()) {
            uint32_t value = 0;

            auto result = std::from_chars(match_code_full.data() + seperator + 1, match_code_full.data() + match_code_full.size(), value);
            if (result.ec != std::errc{})
                return InteractiveGameSetup::invalid("Invalid preference (not a number?)");

            preffered_id = value;
        }
    }

    std::cout << "Setting up game from " << match_code_full << " Singleplayer? " << singleplayer << " preference " << preffered_id.value_or(0xffffffff) << '\n';


    if (!singleplayer) {
        auto match_code = std::string_view{match_code_full}.substr(0, std::min(seperator, match_code_full.size()));
        std::lock_guard _lock(match_lock);
        auto match_iterator = prepared_matches.begin();
        while (match_iterator != prepared_matches.end()) {
            if (match_iterator->code != match_code) {
                ++match_iterator;
                continue;
            }

            auto& match = *match_iterator;

            if (!have_id && preffered_id.value_or(0u) >= match.players.size())
                return InteractiveGameSetup::invalid("Invalid preffered player number");

            auto chosen_spot = match.next_empty_player_slot();
            if (!have_id) {

                ASSERT(chosen_spot < match.players.size());

                if (preffered_id.has_value()) {
                    if (match.is_empty_spot(*preffered_id))
                        chosen_spot = *preffered_id;
                }

                std::cout << "Fitting this player in at " << chosen_spot << '\n';
                match.insert_player(chosen_spot, input, output);
            }

            if (!match.is_ready()) {
                if (!have_id)
                    return InteractiveGameSetup::not_ready(chosen_spot);
                else
                    return InteractiveGameSetup::not_ready();
            }

            if (!match.game_data) {
                match.game_data = match.game->start_game(match.players);
                if (!match.game_data) {
                    // Failed to create game data, safest way to shut everyone down is destroying it
                    match.waiting_observers.close_all("Game could not be setup!");
                    prepared_matches.erase(match_iterator);
                    return InteractiveGameSetup::invalid("Failed to setup game for everyone!");
                }

                match.game_data->set_num_interactive_players(match.interactive_players_left_to_give_state);
            }

            --match.interactive_players_left_to_give_state;

            Observers observers;
            observers.extend_with(match.waiting_observers);

            InteractiveGameSetup setup{
                !have_id ? std::optional<PlayerIdentifier>(chosen_spot) : std::nullopt,
                match.game,
                match.game_data,
                observers,
            };


            if (match.interactive_players_left_to_give_state == 0)
                prepared_matches.erase(match_iterator);

            return setup;
        }

        return InteractiveGameSetup::invalid("Could not find match ");
    }

    // Single player game, serach for game and setup single game

    auto* game_ptr = get_game_ptr(std::string_view{match_code_full}.substr(0, seperator - 1));
    std::cout << "Looking for single player game: " << std::string_view{match_code_full}.substr(0, seperator - 1) << '\n';
    if (!game_ptr)
        return InteractiveGameSetup::invalid("Unknown game for single player");

    auto num_players = game_ptr->get_num_players();

    if (preffered_id.value_or(0) >= num_players)
        return InteractiveGameSetup::invalid("Spot is beyond amount of players for game");

    uint32_t chosen_spot = preffered_id.value_or(0u);

    std::vector<PlayerFactory> players{};
    for (size_t i = 0; i < num_players; ++i) {
        if (chosen_spot == i) {
            players.emplace_back(input, output);
        } else {
            players.emplace_back("");
        }
    }

    InteractiveGameState* game_data = game_ptr->start_game(players);
    if (!game_data)
        return InteractiveGameSetup::invalid("Failed to setup game!");

    game_data->set_num_interactive_players(1);

    return InteractiveGameSetup {
        chosen_spot,
        game_ptr,
        game_data,
        {}
    };
}

InteractiveTickResult tick_interactive_game(InteractiveGame const* game, InteractiveGameState* game_data, PlayerIdentifier called_player_id) {
    std::cout << "My time to shine, running the game!\n";

    // NOTE: It is up to the calling code to call game_data->done_running(), to clear it for running the next time

    if (game_data->game_in_progress()) {
        auto game_tick_result = game->tick_interactive_game(game_data);

        if (game_tick_result.has_value()) {
            if (*game_tick_result == called_player_id)
                return InteractiveTickResult::WaitingOnYou;

            return InteractiveTickResult::Running;
        }
        game_data->set_done_if_in_progress();
        // Fallthrough to clean up
    }

    // Either we are done (already were or just were) or we failed
    bool is_last_player = game_data->mark_player_done_is_last();
    bool failed = game_data->has_failed();
    if (is_last_player)
        return failed ? InteractiveTickResult::FailedCleanUpState : InteractiveTickResult::DoneCleanUpState;

    return failed ? InteractiveTickResult::FailedClearStateOnly : InteractiveTickResult::DoneClearStateOnly;
}

struct WebSocketState {
    std::mutex _lock{};

    enum class State {
        WaitingForStart,
        Starting,
        Running,
        WaitingOnOurInput,
        Done,
        Failed
    };

    State state { State::WaitingForStart };
    std::string match_code{};

    // Input string holds the real value, buffer is just here to lock free add to it!
    std::string input_string;
    std::string input_buffer;
    std::vector<std::string> output_buffer;

    InteractiveGame const* game;
    InteractiveGameState* game_data;
    std::optional<PlayerIdentifier> player_id;
    Observers observers;

    std::vector<std::string> run(bool failed = false) {
        ASSERT(state != State::WaitingForStart);
        ASSERT(state != State::Done);
        ASSERT(state != State::Failed);

        if (state == State::Starting) {
                        ASSERT(!failed);
            auto game_setup = setup_game(player_id.has_value(), match_code, input_string, output_buffer);

            if (game_setup.has_error()) {
                state = State::Failed;
                std::cout << "State failed to setup game reason: " << game_setup.error_message << '\n';
                return {};
            }

            ASSERT(player_id.has_value() || game_setup.player_id.has_value());

            if (game_setup.player_id.has_value())
                player_id = game_setup.player_id;

            if (!game_setup.ready_to_play())
                return {};

            ASSERT(player_id.has_value());

            ASSERT(game_setup.game);
            ASSERT(game_setup.game_data);
            game = game_setup.game;
            game_data = game_setup.game_data;
            state = State::Running;


            game_setup.observers.send_message(crow::json::wvalue {
                {"type", "system"},
                {"content", "connected"}
            }.dump());
            observers.extend_with(game_setup.observers);
        }

        ASSERT((state != State::WaitingForStart) && (state != State::Starting));
        ASSERT(player_id.has_value());
        ASSERT(game);
        ASSERT(game_data);

        // Shortcut, if waiting on us and buffer empty, just get out of here
        if (!failed && state == State::WaitingOnOurInput && input_buffer.empty()) {
            ASSERT(output_buffer.empty());
            return {};
        }

        if (!game_data->can_run_exclusive()) {
            std::cout << "Already running on another call\n";
            if (failed) {
                game_data->mark_failed();

                auto result = tick_interactive_game(game, game_data, *player_id);
                ASSERT((result == InteractiveTickResult::FailedCleanUpState)
                    || (result == InteractiveTickResult::FailedClearStateOnly));

                clean_up_after_game(result);
                ASSERT(state == State::Failed);
            }
            return {};
        } else if (failed) {
            std::cout << "Failing current game (with lock though!)\n";
            game_data->mark_failed();
        }

        state = State::Running;

        // Here is an oppurtunity to update the input!
        if (input_string.empty()) {
            std::swap(input_string, input_buffer);
        } else if (!input_buffer.empty()) {
            input_string += input_buffer;
            input_buffer.clear();
        }

        auto result = tick_interactive_game(game, game_data, *player_id);
        // Anything interactive done with the game data / players should be done while the game is "running"
        {
            Deferred unlock_game_data{[&]{
                game_data->done_running();
            }};

            if (result == InteractiveTickResult::WaitingOnYou) {
                std::cout << "They're waiting on us, quick send the message!\n";
                state = State::WaitingOnOurInput;
                auto result = output_buffer;
                output_buffer.clear();
                return result;
            } else if (result == InteractiveTickResult::Running) {
                // Can't really do anything
                return {};
            }
        }

        clean_up_after_game(result);
        return {};
    }

    void prepare_for_next_game() {
        ASSERT(!game_data);
        observers.close_all();
        game = nullptr;

        output_buffer.clear();
        input_buffer.clear();
        input_string.clear();
        player_id.reset();
        match_code = "";

        // Show we are ready for new game
        state = State::WaitingForStart;
    }
private:
    void clean_up_after_game(InteractiveTickResult result)
    {

        std::cout << "Done with game, with " << static_cast<uint32_t>(result) << '\n';

        // One of the done states
        ASSERT((result == InteractiveTickResult::DoneCleanUpState)
            || (result == InteractiveTickResult::DoneClearStateOnly)
            || (result == InteractiveTickResult::FailedCleanUpState)
            || (result == InteractiveTickResult::FailedClearStateOnly));

        if (result == InteractiveTickResult::DoneCleanUpState || result == InteractiveTickResult::FailedCleanUpState)
            delete game_data;

        game_data = nullptr;

        if (result == InteractiveTickResult::FailedCleanUpState || result == InteractiveTickResult::FailedClearStateOnly)
            state = State::Failed;
        else
            state = State::Done;
    }
};

std::array<WebSocketState, 10> websocketStates{};

WebSocketState* find_next_ws_state(std::string match_code) {
    for (size_t i = 0; i < websocketStates.size(); ++i) {
        std::lock_guard _single_state_lock(websocketStates[i]._lock);
        if (websocketStates[i].state != WebSocketState::State::WaitingForStart)
            continue;

        websocketStates[i].state = WebSocketState::State::Starting;
        websocketStates[i].match_code = std::move(match_code);
        return &websocketStates[i];
    }

    return nullptr;
}

void handle_ws_message(crow::websocket::connection& conn, std::string input, bool closed = false)
{
    ASSERT(conn.userdata());
    auto* ws_state = static_cast<WebSocketState*>(conn.userdata());
    std::lock_guard _lock(ws_state->_lock);

    if (ws_state->state == WebSocketState::State::Done) {
        ASSERT(closed);
        std::cout << "Closing already done!\n";
        return;
    }

    if (ws_state->state == WebSocketState::State::Failed) {
        std::cout << "Closing failed!\n";
        if (!closed) {
            conn.send_text("[{\"type\":\"system\",\"content\":\"Other players quit / Failed!\"}]");

            std::cout << "Closing connection due to failed but not closed\n";
            post_close(conn);
        }

        return;
    }

    if (!input.empty() && !closed) {
        ws_state->input_buffer += input;
        // TODO: Trim string and add new line here!
        // ws_state->input_buffer += '\n';

        std::cout << "Sending input \"" << input << "\" to observers!\n";
        std::cout.flush();
        ws_state->observers.send_message(crow::json::wvalue {
            {"type", "user-message"},
            {"content", input},
        }.dump());
    }

    auto response = ws_state->run(closed);

    if (closed)
        return;

    crow::json::wvalue messages;
    auto push_message = [&messages, message_index = 0u](std::string const& type, std::string content = "") mutable {
        messages[message_index++] = crow::json::wvalue {
            {"type", type},
            {"content", std::move(content)},
        };
    };


    for (auto& str : response) {
        push_message("game-message", std::move(str));
    }

    if (messages.t() == crow::json::type::List && messages.size() > 0) {
        std::cout << "Got " << messages.size() << " messages!\n";
        ws_state->observers.send_message(messages.dump());
    }

    bool close = false;

    if (ws_state->state == WebSocketState::State::WaitingOnOurInput) {
        push_message("you-are-up");
    } else if (ws_state->state == WebSocketState::State::Done) {
        push_message("system", "Game completed!");
        close = true;
    } else if (ws_state->state == WebSocketState::State::Failed) {
        push_message("system", "Failed / Stopped");
        close = true;
    }

    if (messages.t() == crow::json::type::List && messages.size() > 0) {
        conn.send_text(messages.dump());
    }


    if (!close)
        return;

    std::cout << "Closing connection due to done or failed state " << static_cast<int>(ws_state->state) << '\n';
    post_close(conn);
}

struct ObserverState {

    void watch_match(char const* str) {
        ASSERT(m_in_use.test());
        m_watch_code = str;
        m_waiting_on_match = true;
    }

    void watch_id(char const* str) {
        ASSERT(m_in_use.test());
        m_watch_code = str;
        std::cout << "Got watch code: " << m_watch_code << '\n';
        m_waiting_on_match = false;
    }

    bool set_connection(crow::websocket::connection& conn) {
        ASSERT(!m_connection);
        m_connection = std::make_shared<ConnectionHolder>(conn);

        if (m_waiting_on_match) {
            if (add_to_match_queue())
                return true;
        } else {
            if (add_to_player())
                return true;
        }

        // Could not setup!
        m_connection.reset();
        return false;
    }

    void close() {
        ASSERT(m_in_use.test());

        // We don't actually care about resetting the code or type of code as that should be overwritten anyway

        {
            auto lock_ref = m_connection;
            // Yes technically this is a race but this is just a sanity check
            ASSERT(!lock_ref->closed.test());
            lock_ref->closed.test_and_set();

            m_connection.reset();

            while (!lock_ref.unique()) {
                std::this_thread::yield();
            }
        }

        // Finally clear for new use! (Note that even before we exit out of this function it can already be reused)
        m_in_use.clear();
    }

private:

    friend ObserverState* find_next_observer_state();

    bool claim() {
        return !m_in_use.test_and_set();
    }


    bool add_to_player() const {
        size_t value = 0;
        auto result = std::from_chars(m_watch_code.data(), m_watch_code.data() + m_watch_code.size(), value);
        if (result.ec != std::errc{} || result.ptr != (m_watch_code.data() + m_watch_code.size()))
            return false;

        if (value >= websocketStates.size())
            return false;

        auto& wanted_state = websocketStates[value];
        std::lock_guard _lock{wanted_state._lock};

        if (wanted_state.state == WebSocketState::State::WaitingForStart)
            return false;

        ASSERT(wanted_state.state != WebSocketState::State::Done);
        ASSERT(wanted_state.state != WebSocketState::State::Failed);

        wanted_state.observers.add_observer(m_connection);

        return true;
    }


    bool add_to_match_queue() const {
        if (!match_lock.try_lock_for(std::chrono::seconds(15))) {
            std::cout << "Failed to acquire lock for adding observer to match!\n";
            return false;
        }
        Deferred unlock = [&] { match_lock.unlock(); };

        for (auto& match : prepared_matches) {
            if (match.code == m_watch_code) {
                match.waiting_observers.add_observer(m_connection);
                return true;
            }
        }

        return false;
    }


    std::atomic_flag m_in_use;
    bool m_waiting_on_match { false };
    std::string m_watch_code;
    std::shared_ptr<ConnectionHolder> m_connection;
};

std::array<ObserverState, 10> observerStates{};

ObserverState* find_next_observer_state() {
    for (auto& slot : observerStates) {
        if (slot.claim())
            return &slot;
    }
    return nullptr;
}

int main()
{
    srand(time(nullptr));

    games.emplace_back("guess", "Guess", std::make_unique<GuessGame>(5));
    games.emplace_back("ttt", "TickTackToe", std::make_unique<TTTGame>());
    random_engine.seed(rand());

    crow::SimpleApp app;

    CROW_ROUTE(app, "/api/game-info/<string>")([](std::string game) {
        auto* available_game = find_game(game);
        if (!available_game)
            return crow::response(crow::status::NOT_FOUND, "Cannot find game");

        crow::json::wvalue response;
        response["numPlayers"] = available_game->game->get_num_players();
        response["availableAlgos"] = available_game->game->available_algortihms();
        response["gameBaseName"] = available_game->baseName;

        return crow::response{ response };
    });

    CROW_ROUTE(app, "/api/setup-game/<string>")
        .methods(crow::HTTPMethod::POST)
        ([](crow::request const& req, std::string game) -> crow::response {
            auto msg = crow::json::load(req.body);

            if (msg.t() != crow::json::type::List)
                return crow::response(crow::status::BAD_REQUEST, "must be json array of appropiate length");

            std::vector<std::string_view> commands;

            for (size_t i = 0; i < msg.size(); ++i) {
                auto command_view = msg[i].s();
                commands.emplace_back(command_view.begin(), command_view.size());
            }

            auto result = prepare_match(game, commands);
            if (!result.ready)
                return crow::response(crow::status::BAD_REQUEST, std::move(result.code_or_error));

            return crow::json::wvalue {
                {"ready", true},
                {"matchCode", result.code_or_error},
            };
    });


    CROW_WEBSOCKET_ROUTE(app, "/ws-api/game-observe")
            .onaccept([&](crow::request const& req, void** userdata) -> bool {
                auto* user_id = req.url_params.get("user_id");
                auto* match_string = req.url_params.get("match");

                if (!user_id == !match_string)
                    return false;

                if (user_id)
                    std::cout << "Got observer for user " << user_id << '\n';
                else
                    std::cout << "Got observer for match " << match_string << '\n';

                auto* state = find_next_observer_state();
                if (!state) {
                    std::cout << "Could not find open observer slot!\n";
                    return false;
                }

                *userdata = state;

                if (user_id) {
                    std::cout << "Setting watch id\n";
                    state->watch_id(user_id);
                } else {
                    std::cout << "Setting match id\n";
                    ASSERT(match_string);
                    state->watch_match(match_string);
                }

                return true;
            })
            .onopen([&](crow::websocket::connection& conn){
                auto* observe_state = static_cast<ObserverState*>(conn.userdata());
                ASSERT(observe_state);
                if (!observe_state->set_connection(conn)) {
                    std::cout << "Observer failed to setup!\n";
                    conn.send_text("[{\"type\":\"system\",\"content\":\"Observer failed, try again later\"}]");
                    conn.close();
                } else {
                    std::cout << "Observer setup properly: " << &conn << "\n";
                    conn.send_text("[{\"type\":\"system\",\"content\":\"Observer ready, waiting for input\"}]");
                }
            })
            .onmessage([](crow::websocket::connection&, const std::string&, bool) {
                std::cout << "ignoring message from observer!\n";
            })
            .onclose([&](crow::websocket::connection& conn, const std::string&, uint16_t) {
                if (!conn.userdata())
                    return;

                std::cout << "Observer being closed!\n";

                static_cast<ObserverState*>(conn.userdata())->close();
                conn.userdata(nullptr);
            });

    CROW_WEBSOCKET_ROUTE(app, "/ws-api/game-join")
            .onaccept([&](crow::request const& req, void** userdata) -> bool {
                std::cout << "Connection request from: " << req.remote_ip_address << " on " << req.raw_url << '\n';
                auto* match_string = req.url_params.get("match");
                if (!match_string)
                    return false;

                auto* using_state = find_next_ws_state(match_string);
                std::cout << "Got state: " << using_state << '\n';
                if (!using_state)
                    return false;

                std::cout << "Which is index: " << (using_state - websocketStates.data()) << '\n';
                *userdata = using_state;
                return true;
            })
            .onopen([&](crow::websocket::connection& conn){
                std::cout << "Opened ws to " << conn.get_remote_ip() << '\n';
                handle_ws_message(conn, "");
            })
            .onclose([&](crow::websocket::connection& conn, const std::string& reason, uint16_t code){
                std::cout << "Closed ws with " << conn.get_remote_ip() << " for " << reason << '(' << code << ")\n";
                if (!conn.userdata()) {
                    std::cout << "Data already cleared for closing\n";
                    return;
                }

                std::cout << "Sending that we are closing!\n";
                handle_ws_message(conn, "", true);
                std::cout << "Should be closed now!\n";

                {
                    auto* ws_state = static_cast<WebSocketState*>(conn.userdata());
                    std::lock_guard _lock(ws_state->_lock);
                    ASSERT(ws_state->state == WebSocketState::State::Done || ws_state->state == WebSocketState::State::Failed);
                    ws_state->prepare_for_next_game();
                    conn.userdata(nullptr);
                    std::cout << "Cleared userdata!\n";
                }
            })
            .onmessage([&](crow::websocket::connection& conn, const std::string& data, bool is_binary){
                std::cout << "Got message " << data << " from " << &conn << ' ' << is_binary << " binary?\n";

                if (is_binary) {
                    conn.close("Only text supported!", crow::websocket::CloseStatusCode::UnacceptableData);
                    return;
                }

                // FIXME: I think we want to trim the message here?
                handle_ws_message(conn, data);
            });



    auto running =
            app.port(18081)
                       .bindaddr("0.0.0.0")
                       .concurrency(8)
                       .run_async();

    running.wait();
    std::cout << "Running done!\n";
    app.stop();
}

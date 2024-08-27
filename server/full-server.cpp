#include "crow/app.h"
#include "crow/common.h"
#include "crow/json.h"
#include "crow/returnable.h"
#include "crow/websocket.h"
#include <algorithm>
#include <atomic>
#include <cstdint>
#include <string_view>
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
    std::variant<std::string, InteractivePlayerInput> data;

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

struct InteractiveGameSetup {
    std::optional<PlayerIdentifier> player_id;
    InteractiveGame const* game = nullptr;
    InteractiveGameState* game_data = nullptr;

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
            std::move(message),
        };
    }

    static InteractiveGameSetup not_ready(std::optional<PlayerIdentifier> id = std::nullopt) {
        return {
            id, nullptr, nullptr, ""
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

std::mutex match_lock;
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

std::string prepare_match(std::string const& game_name, std::vector<std::string_view> commands) {
    auto* game_ptr = get_game_ptr(game_name);
    if (!game_ptr)
        return "FAIL: Unknown game!";

    if (game_ptr->get_num_players() != commands.size())
        return "FAIL: Invalid number of players!";
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
        return "FAIL: All bot players";

    {
        std::lock_guard _lock(match_lock);

        for (size_t i = 0; i < 25; ++i) {
            if (std::none_of(prepared_matches.begin(), prepared_matches.end(), [&](auto& val) {
                return val.code == match_code;
            })) {
                break;
            } else if (i == 24) {
                return "FAIL: No space available to play game";
            }

            match_code = random_match_code(game_name);
        }

        prepared_matches.emplace_back(interactive_players, match_code, std::move(player_commands), game_ptr);
    }

    return match_code;
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
                    prepared_matches.erase(match_iterator);
                    return InteractiveGameSetup::invalid("Failed to setup game for everyone!");
                }

                match.game_data->set_num_interactive_players(match.interactive_players_left_to_give_state);
            }

            --match.interactive_players_left_to_give_state;

            InteractiveGameSetup setup{
                !have_id ? std::optional<PlayerIdentifier>(chosen_spot) : std::nullopt,
                match.game,
                match.game_data,
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
            std::cout << "State is setup for " << *player_id <<  " starting running!\n";
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

std::mutex websocketStateLock;
std::array<WebSocketState, 10> websocketStates;

WebSocketState* find_next_ws_state(std::string match_code) {
    // FIXME: This can be blocked by long running games!
    std::lock_guard _lock(websocketStateLock);

    for (int i = 0; i < 10; ++i) {
        std::lock_guard _single_state_lock(websocketStates[i]._lock);
        if (websocketStates[i].state != WebSocketState::State::WaitingForStart)
            continue;

        websocketStates[i].state = WebSocketState::State::Starting;
        websocketStates[i].match_code = std::move(match_code);
        return &websocketStates[i];
    }

    return nullptr;
}

void send_ws_game_message(crow::websocket::connection& conn, std::string type, std::string content)
{
    crow::json::wvalue message {
        {"type", type},
        {"content", content}
    };
    conn.send_text(message.dump());
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
            send_ws_game_message(conn, "system", "Other player stopped");

            std::cout << "Closing connection due to failed but not closed\n";
            auto* ptr = dynamic_cast<crow::websocket::Connection<crow::SocketAdaptor, crow::SimpleApp>*>(&conn);
            ASSERT(ptr);
            ptr->post([&]{
                conn.close();
            });
        }

        return;
    }

    if (!input.empty() && !closed)
        ws_state->input_buffer += input;

    auto response = ws_state->run(closed);

    if (closed)
        return;

    for (auto& str : response) {
        send_ws_game_message(conn, "message", str);
}

    if (ws_state->state == WebSocketState::State::WaitingOnOurInput) {
        send_ws_game_message(conn, "you-are-up", "");
        return;
    }

    if (ws_state->state == WebSocketState::State::Done) {
        send_ws_game_message(conn, "system", "Game completed!");
    } else if (ws_state->state == WebSocketState::State::Failed) {
        send_ws_game_message(conn, "system", "Failed / Stopped");
    } else {
        return;
    }

    std::cout << "Closing connection due to done or failed state " << static_cast<int>(ws_state->state) << '\n';
    auto* ptr = dynamic_cast<crow::websocket::Connection<crow::SocketAdaptor, crow::SimpleApp>*>(&conn);
    ASSERT(ptr);
    ptr->post([&]{
        conn.close();
    });
}

int main()
{
    srand(time(nullptr));

    games.emplace_back("guess", "Guess", std::make_unique<GuessGame>(5));
    random_engine.seed(rand());

    crow::SimpleApp app;

    {
        std::string command = "internal";
        std::vector<std::string_view> commands {
            command,
            {},
            command,
            command,
            {},
        };
        auto match_name = prepare_match("guess", commands);
        std::cout << "match ready with code: " << match_name << '\n';
    }

    CROW_ROUTE(app, "/api/game-info/<string>")([](crow::response& res, std::string game) {
        auto* available_game = find_game(game);
        res.add_header("Content-Type", "application/json");
        if (!available_game) {
            res.code = crow::status::NOT_FOUND;

            res.write(crow::json::wvalue {
                {"failed", true},
                {"error", "Unknown game"}
            }.dump());
            res.end();
            return;
        }

        crow::json::wvalue response;
        response["numPlayers"] = available_game->game->get_num_players();
        response["availableAlgos"] = available_game->game->available_algortihms();
response["gameBaseName"] = available_game->baseName;

        res.write(response.dump());
        res.end();
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

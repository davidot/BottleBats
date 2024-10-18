#pragma once

#include "../../util/Assertions.h"
#include "StringChannel.h"
#include <atomic>
#include <cstdint>
#include <memory>
#include <variant>
#include <vector>

namespace BBServer {

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
    bool game_in_progress();

    void set_done_if_in_progress();

    void mark_failed();

    bool has_failed();

    bool can_run_exclusive();

    void done_running();

    bool mark_player_done_is_last();

    void add_error(PlayerIdentifier player, std::string&& message);
    void move_errors(PlayerIdentifier for_player, std::vector<std::string>& output);

    virtual ~InteractiveGameState() = 0;

    // FIXME: Protect me with friend or something
    void set_num_interactive_players(uint32_t amount);

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
    struct PlayerError {
        PlayerIdentifier player_id;
        std::string message;
    };
    std::vector<PlayerError> m_errors;
};

struct PlayerFactory {
    struct InteractivePlayerInput {
        std::string& input;
        std::vector<std::string>& output;
    };

    explicit PlayerFactory(std::string command_)
        : data(std::move(command_))
    {
    }

    explicit PlayerFactory(std::string& input, std::vector<std::string>& output)
        : data(InteractivePlayerInput { input, output })
    {
    }

    bool is_interactive() const;

    bool contains_input(std::string const& other) const;

    template<typename PlayerType>
    std::unique_ptr<PlayerType> construct_interactive() const
    {
        ASSERT(is_interactive());
        auto& references = std::get<InteractivePlayerInput>(data);
        return std::make_unique<PlayerType>(StringCommunicator(references.input, references.output));
    }

    void override_interactive_player(std::string& input, std::vector<std::string>& output);

    std::string const& command() const;

private:
    std::variant<std::string, InteractivePlayerInput> data;
};

struct InteractiveGameTickResult {
    [[nodiscard]] bool is_done()
    {
        return m_finished;
    }

    [[nodiscard]] bool has_error()
    {
        return !m_error.empty();
    }

    std::string const& error()
    {
        return m_error;
    }

    std::string&& extract_string()
    {
        return std::move(m_error);
    }

    PlayerIdentifier get_next_player() const
    {
        return m_waitingOnPlayer;
    }

    InteractiveGameTickResult();

    InteractiveGameTickResult(size_t player, std::string error, bool finished = false);

private:
    bool m_finished;
    PlayerIdentifier m_waitingOnPlayer;
    std::string m_error;
};

template<typename T>
struct ContinuableResult {
    enum class Type {
        Valid,
        WaitingForValidMove,
        InvalidMove
    } type;

    ContinuableResult(StringReaderResult result)
        : m_type(result.can_continue ? Type::WaitingForValidMove : Type::InvalidMove)
        , m_message(std::move(result.error))
    {
        ASSERT(result.failed);
    }

    template<typename... Args>
    ContinuableResult(Args&&... args)
        : m_result(std::forward<Args>(args)...)
        , m_type(Type::Valid)
        , m_message("")
    {
    }

    bool has_result() const
    {
        return m_type == Type::Valid;
    }

    bool can_continue() const
    {
        return m_type != Type::InvalidMove;
    }

    std::string const& error()
    {
        ASSERT(m_type != Type::Valid);
        return m_message;
    }

    T& result()
    {
        ASSERT(m_type == Type::Valid);
        return m_result;
    }

    InteractiveGameTickResult to_tick_result(PlayerIdentifier player)
    {
        ASSERT(m_type != Type::Valid);
        return InteractiveGameTickResult {
            player,
            std::move(m_message),
            m_type == Type::InvalidMove
        };
    }

private:
    T m_result {};
    Type m_type;
    std::string m_message;
};

struct InteractiveGame {
    virtual ~InteractiveGame() { }

    virtual uint32_t get_num_players() const = 0;
    virtual std::vector<std::string> const& available_algortihms() const = 0;
    virtual InteractiveGameState* start_game(std::vector<PlayerFactory> const& setup) const = 0;

    virtual InteractiveGameTickResult tick_interactive_game(InteractiveGameState* game_data) const = 0;
};

}
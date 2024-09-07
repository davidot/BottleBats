#pragma once

#include <cstdint>
#include <atomic>
#include <vector>
#include <optional>
#include <memory>
#include <variant>
#include "../../util/Assertions.h"

namespace BBServer {

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
};



using PlayerIdentifier = uint32_t;

struct PlayerFactory {
    struct InteractivePlayerInput {
        std::string& input;
        std::vector<std::string>& output;
    };

    explicit PlayerFactory(std::string command_)
        : data(std::move(command_)) {}

    explicit PlayerFactory(std::string& input, std::vector<std::string>& output)
        : data(InteractivePlayerInput{input, output}) {}

    bool is_interactive() const;

    bool contains_input(std::string const& other) const;

    template<typename PlayerType>
    std::unique_ptr<PlayerType> construct_interactive() const {
        ASSERT(is_interactive());
        auto& references = std::get<InteractivePlayerInput>(data);
        return std::make_unique<PlayerType>(references.input, references.output);
    }

    void override_interactive_player(std::string& input, std::vector<std::string>& output);

    std::string const& command() const;

private:
    std::variant<std::string, InteractivePlayerInput> data;
};

struct InteractiveGame {
    virtual ~InteractiveGame() {}

    virtual uint32_t get_num_players() const = 0;
    virtual std::vector<std::string> const& available_algortihms() const = 0;
    virtual InteractiveGameState* start_game(std::vector<PlayerFactory> const& setup) const = 0;

    virtual std::optional<PlayerIdentifier> tick_interactive_game(InteractiveGameState* game_data) const = 0;
};

}
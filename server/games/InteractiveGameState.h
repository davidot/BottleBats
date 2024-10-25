#pragma once

#include <atomic>
#include <cstdint>
#include <vector>
#include <string>

namespace BBServer {

using PlayerIdentifier = uint32_t;

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


}
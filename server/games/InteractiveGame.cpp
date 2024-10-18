#include "InteractiveGame.h"
#include <algorithm>

namespace BBServer {

InteractiveGameState::~InteractiveGameState() { }

bool PlayerFactory::is_interactive() const
{
    return !std::holds_alternative<std::string>(data);
}

bool PlayerFactory::contains_input(std::string const& other) const
{
    ASSERT(is_interactive());
    return &(std::get<InteractivePlayerInput>(data).input) == &other;
}

void PlayerFactory::override_interactive_player(std::string& input, std::vector<std::string>& output)
{
    ASSERT(is_interactive());
    data.emplace<InteractivePlayerInput>(input, output);
}

std::string const& PlayerFactory::command() const
{
    ASSERT(!is_interactive());
    return std::get<std::string>(data);
}

void InteractiveGameState::set_done_if_in_progress()
{
    DoneState expected { DoneState::Running };
    bool exchanged = done.compare_exchange_strong(expected, DoneState::Done);
    ASSERT(exchanged || expected == DoneState::Failed);
}

bool InteractiveGameState::game_in_progress()
{
    return done.load() == DoneState::Running;
}

void InteractiveGameState::mark_failed()
{
    done.store(DoneState::Failed);
}

bool InteractiveGameState::has_failed()
{
    return done.load() == DoneState::Failed;
}

bool InteractiveGameState::can_run_exclusive()
{
    return !run_lock.test_and_set();
}

void InteractiveGameState::done_running()
{
    run_lock.clear();
}

bool InteractiveGameState::mark_player_done_is_last()
{
    auto old_value = active_interactive_players.fetch_sub(1);
    ASSERT(old_value >= 1);
    return old_value == 1;
}

void InteractiveGameState::set_num_interactive_players(uint32_t amount)
{
    active_interactive_players.store(amount);
}

void InteractiveGameState::move_errors(PlayerIdentifier for_player, std::vector<std::string>& output)
{
    auto remove_from = std::remove_if(m_errors.begin(), m_errors.end(),
        [for_player, &output](PlayerError const& error) {
            if (error.player_id != for_player)
                return false;
            output.emplace_back(std::move(error.message));
            return true;
        });

    m_errors.erase(remove_from, m_errors.end());
}

void InteractiveGameState::add_error(PlayerIdentifier player, std::string&& message)
{
    m_errors.emplace_back(player, message);
}

InteractiveGameTickResult::InteractiveGameTickResult()
    : m_finished(true)
    , m_waitingOnPlayer(0)
    , m_error("")
{
}

InteractiveGameTickResult::InteractiveGameTickResult(size_t player, std::string error, bool finished)
    : m_finished(finished)
    , m_waitingOnPlayer(player)
    , m_error(std::move(error))
{
}

}

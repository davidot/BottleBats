#include "InteractiveGameTickResult.h"

namespace BBServer {

bool InteractiveGameTickResult::is_done()
{
    return m_finished;
}

bool InteractiveGameTickResult::has_error()
{
    return !m_error.empty();
}

std::string const& InteractiveGameTickResult::error()
{
    return m_error;
}

std::string&& InteractiveGameTickResult::extract_string()
{
    return std::move(m_error);
}

PlayerIdentifier InteractiveGameTickResult::get_next_player() const
{
    return m_waitingOnPlayer;
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
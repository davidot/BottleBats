#pragma once

#include <string>
#include "InteractiveGameState.h"
#include "StringChannel.h"

namespace BBServer {


struct InteractiveGameTickResult {
    [[nodiscard]] bool is_done();

    [[nodiscard]] bool has_error();

    std::string const& error();

    std::string&& extract_string();

    PlayerIdentifier get_next_player() const;

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


}
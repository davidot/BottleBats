#pragma once

#include <utility>

template<typename Callback>
class Deferred {
public:
    Deferred(Callback function)
        : m_function(std::move(function))
    {
    }

    ~Deferred() {
        m_function();
    }

private:
    Callback m_function;
};

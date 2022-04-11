#pragma once

#include <functional>

class Deferred {
public:
    Deferred(std::function<void()>&& function)
        : m_function(function)
    {
    }

    ~Deferred() {
        m_function();
    }

private:
    std::function<void()> m_function;
};

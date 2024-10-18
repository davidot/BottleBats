#pragma once
#include "../util/Assertions.h"
#include <array>
#include <numeric>
#include <string>
#include <string_view>
#include <utility>

namespace BBServer {

template<typename T, size_t N>
struct StringMapper {
    std::string_view to_string(T value) const
    {
        for (const auto& [val, vw] : m_mapping) {
            if (val == value)
                return vw;
        }
        ASSERT(false);
        return "";
    }

    bool from_string(T& value, std::string_view view) const
    {
        for (const auto& [val, vw] : m_mapping) {
            if (view == vw) {
                value = val;
                return true;
            }
        }
        return false;
    }

    std::string all_options() const
    {
        return std::accumulate(m_mapping.begin(), m_mapping.end(), std::string(),
            [](const std::string& a, const std::pair<T, std::string_view>& b) -> std::string {
                return a + "," + b;
            });
    }

    std::array<std::pair<T, std::string_view>, N> m_mapping;
};

template<typename T, typename... Args>
constexpr auto enum_to_string_mapper(T value, Args... args)
{
    static_assert(sizeof...(args) % 2 == 1, "Arguments must be in pairs of value and string_view.");

    // This code is quite messy but allows the mapper being constructed like:
    // enum_to_string_mapper(Enum::Val1, "a", Enum::Val2, "b", ...)
    constexpr size_t N = (sizeof...(args) + 1) / 2;
    std::array<T, N> values {};
    std::array<std::string_view, N> views {};

    values[0] = value;

    size_t index = 0;

    ([&] {
        ++index;

        if constexpr (std::is_same_v<decltype(args), T>)
            values[index / 2] = args;
        else
            views[index / 2] = std::string_view(args);
    }(),
        ...);

    std::array<std::pair<T, std::string_view>, N> zipped {};
    for (size_t i = 0; i < N; ++i) {
        zipped[i] = std::make_pair(values[i], views[i]);
    }

    return StringMapper<T, N>(zipped);
}

}
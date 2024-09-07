#pragma once

#include <memory>
#include <tuple>
#include <type_traits>
#include <vector>
#include <string>

namespace BBServer {

template<typename BaseType, typename DefaultType, typename ...Types>
struct SimplePlayerCreator {
    static_assert(std::is_base_of_v<BaseType, DefaultType>, "All DefaultType must derive from BaseType");
    static_assert(((std::is_base_of_v<BaseType, Types>) && ...), "All Types must derive from BaseType");

    std::vector<std::string> const& names() {
        return m_names;
    }

    std::unique_ptr<BaseType> create(std::string const& command) {
        return create_on_index<>(command);
    }

    SimplePlayerCreator() {
        m_names.reserve(sizeof...(Types) + 1);
        (m_names.emplace_back(Types::name), ...);
        m_names.emplace_back(DefaultType::name);
    }

private:
    using TypeStorage = std::tuple<Types..., DefaultType>;

    template<size_t Index = 0>
    std::unique_ptr<BaseType> create_on_index(std::string const& command) {
        constexpr size_t BoundedIndex = std::min(Index, sizeof...(Types));
        using TypeAtIndex = std::tuple_element_t<BoundedIndex, TypeStorage>;
        static_assert((Index >= sizeof...(Types)) == std::is_same_v<TypeAtIndex, DefaultType>, "Default type should be last only!");

        if (command == m_names[Index] || Index >= sizeof...(Types))
            return std::make_unique<TypeAtIndex>();

        return create_on_index<BoundedIndex + 1>(command);
    }

    std::vector<std::string> m_names{};

};

}
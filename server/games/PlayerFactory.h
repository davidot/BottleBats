#pragma once

#include <variant>
#include <string>
#include <vector>
#include <memory>
#include "StringChannel.h"

namespace BBServer {


struct PlayerFactory {
    explicit PlayerFactory(std::string command_);

    explicit PlayerFactory(std::string& input, std::vector<std::string>& output);

    bool is_internal() const;

    bool is_interactive() const;

    bool is_command() const;

    bool contains_input(std::string const& other) const;

    StringCommunicator to_communicator() const;

    template<typename PlayerType>
    std::unique_ptr<PlayerType> construct_string_based() const
    {
        ASSERT(is_interactive() || is_command());
        return std::make_unique<PlayerType>(to_communicator());
    }

    void override_interactive_player(std::string& input, std::vector<std::string>& output);

    std::string const& command() const;

private:
    struct InteractivePlayerInput {
        std::string& input;
        std::vector<std::string>& output;
    };

    std::variant<std::string, InteractivePlayerInput, std::vector<std::string>> data;
};


}
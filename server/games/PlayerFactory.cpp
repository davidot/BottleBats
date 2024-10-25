#include "PlayerFactory.h"

namespace BBServer {

bool PlayerFactory::is_internal() const
{
    return std::holds_alternative<std::string>(data);
}

bool PlayerFactory::is_interactive() const
{
    return std::holds_alternative<InteractivePlayerInput>(data);
}

bool PlayerFactory::is_command() const
{
    return std::holds_alternative<std::vector<std::string>>(data);
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

StringCommunicator PlayerFactory::to_communicator() const
{
    if (is_interactive()) {
        auto& references = std::get<InteractivePlayerInput>(data);
        return { references.input, references.output };
    }
    ASSERT(is_command());
    auto& references = std::get<std::vector<std::string>>(data);
    return StringCommunicator { util::SubProcess::create(references) };
}

PlayerFactory::PlayerFactory(std::string& input, std::vector<std::string>& output)
    : data(InteractivePlayerInput { input, output })
{
}

PlayerFactory::PlayerFactory(std::string command_)
    : data(std::move(command_))
{
}

}
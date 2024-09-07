#include "TickTackToe.h"
#include "../PlayerFactoryHelper.h"
#include <iostream>
#include <charconv>
#include <sstream>

namespace BBServer::TickTackToe {

std::optional<size_t> InteractiveTTTPlayer::play(std::array<FieldValue, 9> const& field, FieldValue you)
{
    if (!sent_output) {
        sent_output = true;
        std::ostringstream message {};
        ASSERT(you == FieldValue::Circle || you == FieldValue::Cross);
        message << "turn " << to_char(you) << ' ';
        for (size_t i = 0; i < field.size(); ++i) {
            if (i > 0 && i % 3 == 0)
                message << '|';
            message << to_char(field[i]);
        }
        m_output_buffer.emplace_back(message.str());
    }

    auto end_of_line = m_input_buffer.find('\n');
    if (end_of_line == std::string::npos)
        return std::nullopt;

    int value = -1;
    auto result = std::from_chars(m_input_buffer.data(), m_input_buffer.data() + end_of_line, value);
    if (result.ec != std::errc {})
        return std::nullopt; // INVALID!! STOP HERE!

    if (value < 0 || value > 8)
        return std::nullopt; // INVALID!! STOP HERE!

    if (field[value] != TTTPlayer::FieldValue::Empty)
        return std::nullopt; // INVALID!! STOP HERE!

    sent_output = false;
    m_input_buffer.erase(0, end_of_line + 1);

    return value;
}

std::optional<PlayerIdentifier> BBServer::TickTackToe::TTTGame::tick_game_state(TTTGameState& game_state) const
{

    while (true) {
        PlayerIdentifier this_player = game_state.turnForPlayer;
        auto& player = game_state.players[game_state.turnForPlayer];
        TTTGameState::FieldValue player_symbol = game_state.player_mapping[this_player];
        auto potential_guess = player->play(game_state.field, player_symbol);
        if (!potential_guess.has_value()) {
            std::cout << "Player " << this_player << " has no move ready waiting...\n";
            return this_player;
        }

        size_t guess = *potential_guess;
        if (game_state.field[guess] != TTTPlayer::FieldValue::Empty) {
            // Player misbehaved!!! Somehow communicate this
            break;
        }
        game_state.field[guess] = player_symbol;

        if (game_state.win_for(player_symbol)) {
            break;
        }

        game_state.turnForPlayer = (game_state.turnForPlayer + 1) % game_state.players.size();
    }

    return {};
}

class FirstSpotPlayer : public TTTPlayer {
public:
    static constexpr auto name = "first";

    std::optional<size_t> play(std::array<FieldValue, 9> const& field, FieldValue) override {
        for (size_t i = 0; i < field.size(); ++i) {
            if (field[i] == FieldValue::Empty)
                return i;
        }
        ASSERT(false);
    }
};

class LastSpotPlayer : public TTTPlayer {
public:
    static constexpr auto name = "last";

    std::optional<size_t> play(std::array<FieldValue, 9> const& field, FieldValue) override {
        for (size_t i = field.size(); i > 0; --i) {
            if (field[i - 1] == FieldValue::Empty)
                return i - 1;
        }
        ASSERT(false);
    }
};


static SimplePlayerCreator<TTTPlayer, FirstSpotPlayer, LastSpotPlayer> creator;

std::vector<std::string> const& TTTGame::available_algortihms() const
{
    return creator.names();
}

std::unique_ptr<TTTPlayer> TTTGame::player_from_command(std::string const& command) const
{
    return creator.create(command);
}

static std::array<std::tuple<uint8_t, uint8_t, uint8_t>, 8> constexpr winning_lines = {{
    {0, 1, 2},
    {3, 4, 5},
    {6, 7, 8},

    {0, 3, 6},
    {1, 4, 7},
    {2, 5, 8},

    {0, 4, 8},
    {2, 4, 6}
}};

bool TTTGameState::triple_for(FieldValue symbol) const
{
    for (auto& [a, b, c] : winning_lines) {
        if (field[a] == symbol && field[b] == symbol && field[c] == symbol)
            return true;
    }
    return false;
}
TTTGameState::FieldValue TTTGameState::invert(FieldValue value)
{
    switch (value) {
    case FieldValue::Circle:
        return FieldValue::Cross;
    case FieldValue::Cross:
        return FieldValue::Circle;
    case FieldValue::Empty:
        return FieldValue::Empty;
    }
    ASSERT(false);
}
bool TTTGameState::win_for(FieldValue symbol) const
{
    ASSERT(symbol == FieldValue::Cross || symbol == FieldValue::Circle);
    ASSERT(!triple_for(invert(symbol)));
    return triple_for(symbol);
}
char TTTPlayer::to_char(FieldValue value)
{
    switch (value) {
    case FieldValue::Empty:
        return '-';
    case FieldValue::Cross:
        return 'X';
    case FieldValue::Circle:
        return 'O';
    }
    ASSERT(false);
}
TTTGameState* TTTGame::game_for_players(std::array<std::unique_ptr<TTTPlayer>, 2> players) const
{
    return new TTTGameState {
        std::move(players), TTTGameState::FieldValue::Cross
    };
};
}
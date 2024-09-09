#include "Connect4.h"
#include "../PlayerFactoryHelper.h"
#include <iostream>
#include <charconv>
#include <sstream>

namespace BBServer::Connect4 {

std::optional<size_t> InteractiveConnect4Player::play(std::array<FieldValue, FieldSize> const& field, FieldValue you)
{
    if (!sent_output) {
        sent_output = true;
        std::ostringstream message {};
        ASSERT(you == FieldValue::Circle || you == FieldValue::Cross);
        message << "turn " << to_char(you) << ' ';
        for (size_t i = 0; i < field.size(); ++i) {
            if (i > 0 && i % RowSize == 0)
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

    if (field[value] != Connect4Player::FieldValue::Empty)
        return std::nullopt; // INVALID!! STOP HERE!

    sent_output = false;
    m_input_buffer.erase(0, end_of_line + 1);

    return value;
}

std::optional<PlayerIdentifier> Connect4Game::tick_game_state(Connect4GameState& game_state) const
{

    while (true) {
        PlayerIdentifier this_player = game_state.turnForPlayer;
        auto& player = game_state.players[game_state.turnForPlayer];
        Connect4GameState::FieldValue player_symbol = game_state.player_mapping[this_player];
        auto potential_column = player->play(game_state.field, player_symbol);
        if (!potential_column.has_value()) {
            std::cout << "Player " << this_player << " has no move ready waiting...\n";
            return this_player;
        }

        size_t column = *potential_column;
        if (game_state.field[column] != Connect4Player::FieldValue::Empty) {
            // Player misbehaved!!! Somehow communicate this
            break;
        }
        game_state.drop(column, player_symbol);

        if (game_state.win_for(player_symbol)) {
            break;
        }

        game_state.turnForPlayer = (game_state.turnForPlayer + 1) % game_state.players.size();
    }

    return {};
}

class FirstSpotPlayer : public Connect4Player {
public:
    static constexpr auto name = "first";

    std::optional<size_t> play(std::array<FieldValue, FieldSize> const& field, FieldValue) override {
        for (size_t i = 0; i < RowSize; ++i) {
            if (field[i] == FieldValue::Empty)
                return i;
        }
        ASSERT(false);
    }
};

class LastSpotPlayer : public Connect4Player {
public:
    static constexpr auto name = "last";

    std::optional<size_t> play(std::array<FieldValue, FieldSize> const& field, FieldValue) override {
        for (size_t i = RowSize; i > 0; --i) {
            if (field[i - 1] == FieldValue::Empty)
                return i - 1;
        }
        ASSERT(false);
    }
};


static SimplePlayerCreator<Connect4Player, FirstSpotPlayer, LastSpotPlayer> creator;

std::vector<std::string> const& Connect4Game::available_algortihms() const
{
    return creator.names();
}

std::unique_ptr<Connect4Player> Connect4Game::player_from_command(std::string const& command) const
{
    return creator.create(command);
}

void Connect4GameState::drop(size_t column, FieldValue symbol) {
    ASSERT(column < RowSize);
    ASSERT(field[column] == Connect4Player::FieldValue::Empty);

    size_t row = 0;
    while ((row + 1) * RowSize + column < FieldSize) {
        if (field[(row + 1) * RowSize + column] != Connect4Player::FieldValue::Empty)
            break;
        
        ++row;
    }

    field[row * RowSize + column] = symbol;
}


bool Connect4GameState::quad_for(FieldValue symbol) const
{
    for (size_t row = 0; row < NumRows; ++row) {
        size_t row_streak = 0;
        size_t col_streak = 0;

        for (size_t col = 0; col < RowSize; ++col) {
            FieldValue val = field[row * RowSize + col];
            if (row == 0) {
                if (val == symbol) {
                    if (++col_streak >= 4)
                        return true;
                } else {
                    col_streak = 0;
                }
            }

            if (val == symbol) {
                if (++row_streak >= 4)
                    return true;
            } else {
                row_streak = 0;
            }
        }
    }

    return false;
}

Connect4GameState::FieldValue Connect4GameState::invert(FieldValue value)
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

bool Connect4GameState::win_for(FieldValue symbol) const
{
    ASSERT(symbol == FieldValue::Cross || symbol == FieldValue::Circle);
    ASSERT(!quad_for(invert(symbol)));
    return quad_for(symbol);
}

char Connect4Player::to_char(FieldValue value)
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

Connect4GameState* Connect4Game::game_for_players(std::array<std::unique_ptr<Connect4Player>, 2> players) const
{
    return new Connect4GameState {
        std::move(players), Connect4GameState::FieldValue::Cross
    };
};
}
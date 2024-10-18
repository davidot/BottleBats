#include "Connect4.h"
#include "../PlayerFactoryHelper.h"
#include <charconv>
#include <iostream>
#include <sstream>

namespace BBServer::Connect4 {

Connect4Move InteractiveConnect4Player::play(std::array<FieldValue, FieldSize> const& field, FieldValue you)
{
    if (auto writer = m_communicator.output_writer(StringCommunicator::OncePerInput); writer.will_output()) {
        ASSERT(you == FieldValue::Circle || you == FieldValue::Cross);
        writer << "turn " << to_char(you) << ' ';
        for (size_t i = 0; i < field.size(); ++i) {
            if (i > 0 && i % RowSize == 0)
                writer << '|';
            writer << to_char(field[i]);
        }
    }

    auto reader = m_communicator.input_reader(1000);
    if (auto error = reader.has_line(); error.failed)
        return error;

    uint32_t value = -1;
    if (auto error = reader.read_int(value, 0u, RowSize - 1); error.failed)
        return error;

    if (field[value] != Connect4Player::FieldValue::Empty)
        return reader.error(std::string("Column ") + std::to_string(value) + " already full");

    return value;
}

InteractiveGameTickResult Connect4Game::tick_game_state(Connect4GameState& game_state) const
{

    while (true) {
        PlayerIdentifier this_player = game_state.turnForPlayer;
        auto& player = game_state.players[game_state.turnForPlayer];
        Connect4GameState::FieldValue player_symbol = game_state.player_mapping[this_player];
        auto potential_column = player->play(game_state.field, player_symbol);
        if (!potential_column.has_result())
            return potential_column.to_tick_result(this_player);

        size_t column = potential_column.result();
        if (game_state.field[column] != Connect4Player::FieldValue::Empty) {
            return {
                this_player,
                "Dropped on already full column!",
            };
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

    Connect4Move play(std::array<FieldValue, FieldSize> const& field, FieldValue) override
    {
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

    Connect4Move play(std::array<FieldValue, FieldSize> const& field, FieldValue) override
    {
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

void Connect4GameState::drop(size_t column, FieldValue symbol)
{
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
    // Vertical check
    for (size_t col = 0; col < RowSize; ++col) {
        size_t col_streak = 0;
        for (size_t row = 0; row < NumRows; ++row) {
            ASSERT(row * RowSize + col < field.size());

            FieldValue val = field[row * RowSize + col];

            if (val == symbol) {
                if (++col_streak >= 4)
                    return true;
            } else {
                col_streak = 0;
            }
        }
    }

    // Horizontal check
    for (size_t row = 0; row < NumRows; ++row) {
        size_t row_streak = 0;
        for (size_t col = 0; col < RowSize; ++col) {
            ASSERT(row * RowSize + col < field.size());

            FieldValue val = field[row * RowSize + col];

            if (val == symbol) {
                if (++row_streak >= 4)
                    return true;
            } else {
                row_streak = 0;
            }
        }
    }

    for (size_t row = 0; row < NumRows; ++row) {
        for (size_t col = 0; col < RowSize - 3; ++col) {
            ASSERT(row * RowSize + col < field.size());

            if (field[row * RowSize + col] != symbol)
                continue;

            size_t up_streak = 0;
            size_t down_streak = 0;

            for (size_t off = 1; off < 4; ++off) {
                ASSERT(col + off < RowSize);
                if (row + off < NumRows) {
                    FieldValue val = field[(row + off) * RowSize + (col + off)];
                    if (val == symbol) {
                        if (++up_streak >= 3)
                            return true;
                    } else {
                        up_streak = 0;
                    }
                }

                if (off <= row) {
                    FieldValue val = field[(row - off) * RowSize + (col + off)];
                    if (val == symbol) {
                        if (++down_streak >= 3)
                            return true;
                    } else {
                        down_streak = 0;
                    }
                }
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
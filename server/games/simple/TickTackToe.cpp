#include "TickTackToe.h"
#include "../PlayerFactoryHelper.h"
#include <iostream>
#include <memory>

namespace BBServer::TickTackToe {

TTTMove InteractiveTTTPlayer::play(std::array<FieldValue, 9> const& field, FieldValue you)
{
    if (auto writer = m_communicator.output_writer(StringCommunicator::OncePerInput); writer.will_output()) {
        ASSERT(you == FieldValue::Circle || you == FieldValue::Cross);
        writer << "turn " << to_char(you) << ' ';
        for (size_t i = 0; i < field.size(); ++i) {
            if (i > 0 && i % 3 == 0)
                writer << '|';
            writer << to_char(field[i]);
        }
    }

    auto reader = m_communicator.input_reader(500);
    if (auto error = reader.has_line(); error.failed)
        return error;

    int value = -1;
    if (auto error = reader.read_int(value, 0, 8); error.failed)
        return error;

    if (field[value] != TTTPlayer::FieldValue::Empty)
        return reader.error("Bot filled value already filled");

    return value;
}

InteractiveGameTickResult BBServer::TickTackToe::TTTGame::tick_game_state(TTTGameState& game_state) const
{

    while (true) {
        PlayerIdentifier this_player = game_state.turnForPlayer;
        auto& player = game_state.players[game_state.turnForPlayer];
        TTTGameState::FieldValue player_symbol = game_state.player_mapping[this_player];
        auto potential_guess = player->play(game_state.field, player_symbol);
        if (!potential_guess.has_result())
            return potential_guess.to_tick_result(this_player);

        size_t guess = potential_guess.result();
        if (game_state.field[guess] != TTTPlayer::FieldValue::Empty)
            return { this_player, "Already filled spot being filled" };

        game_state.field[guess] = player_symbol;

        if (game_state.win_for(player_symbol))
            break;

        game_state.turnForPlayer = (game_state.turnForPlayer + 1) % game_state.players.size();
    }

    return {};
}

class FirstSpotPlayer : public TTTPlayer {
public:
    static constexpr auto name = "first";

    TTTMove play(std::array<FieldValue, 9> const& field, FieldValue) override
    {
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

    TTTMove play(std::array<FieldValue, 9> const& field, FieldValue) override
    {
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

static std::array<std::tuple<uint8_t, uint8_t, uint8_t>, 8> constexpr winning_lines = { { { 0, 1, 2 },
    { 3, 4, 5 },
    { 6, 7, 8 },

    { 0, 3, 6 },
    { 1, 4, 7 },
    { 2, 5, 8 },

    { 0, 4, 8 },
    { 2, 4, 6 } } };

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

InteractiveTTTPlayer::InteractiveTTTPlayer(StringCommunicator communicator)
    : m_communicator(std::move(communicator))
{
}

}
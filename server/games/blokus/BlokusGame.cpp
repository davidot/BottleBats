#include "BlokusGame.h"
#include "../PlayerFactoryHelper.h"
#include <cstdint>
#include <algorithm>


namespace BBServer::Blokus {


size_t piece_letter_to_index(char c)
{
    for (size_t i = 0; i < ALL_PIECES.size(); ++i) {
        if (ALL_PIECES[i].letter == c)
            return i;
    }
    return ALL_PIECES.size();
}

struct RandomBlokusPlayer : public BlokusPlayer {
    static constexpr auto name = "random-move";
};

struct FirstBlokusPlayer : public BlokusPlayer {
    static constexpr auto name = "first1";
};

static SimplePlayerCreator<BlokusPlayer, RandomBlokusPlayer, FirstBlokusPlayer> creator;

std::vector<std::string> const& available_algortihms()
{
    return creator.names();
}

std::unique_ptr<BlokusPlayer> player_from_command(std::string const& command)
{
    return creator.create(command);
}

InteractiveGameTickResult tick_game_state(AnyBoard& board, uint8_t& turn, std::span<PlayerState> player_states, std::span<std::unique_ptr<BlokusPlayer>> players)
{
    while (true) {
        auto& player = players[turn];

        BoardAtMove board_at_move{board, turn};

        if (!board_at_move.has_move(player_states[turn] == PlayerState::InitialTurn)) {
            player_states[turn] = PlayerState::Passed;
            if (std::all_of(player_states.begin(), player_states.end(), [](PlayerState state) { return state == PlayerState::Passed; })) {
                return {};
            }
        }

        auto potential_move = player->play(std::move(board_at_move));

        if (!potential_move.has_result())
            return potential_move.to_tick_result(turn);

        auto& made_move = potential_move.result();
        auto result = board.place_piece(made_move.piece_index, turn, made_move.rotation, made_move.top, made_move.left);
        if (result != PlaceResult::Placed)
            return { turn, "Wrong!" }; // FIXME: Make nicer error message

        turn = (turn + 1) % players.size();
    }

    return {};
}

bool BoardAtMove::has_move(bool is_initial_move) {

}


}
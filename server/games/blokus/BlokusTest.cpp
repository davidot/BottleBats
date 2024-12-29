#include "BlokusGame.h"
#include "BlokusPieces.h"
#include <memory>

using namespace BBServer::Blokus;

struct RandomPlayer : public BlokusPlayer {
    BlokusMove play(BoardAtMove at_move) override
    {
        auto& moves = at_move.all_moves();
        ASSERT(moves.size() > 0);
        return moves[rand() % moves.size()];
    }
};

int main()
{
    srand(time(nullptr));

    BlokusState<2> game_state { { std::make_unique<RandomPlayer>(), std::make_unique<RandomPlayer>() } };
    BlokusGame<2> game {};
    auto res = game.tick_game_state(game_state);
    std::cout << "game-res:" << res.has_error() << " and " << res.is_done() << '\n';
}
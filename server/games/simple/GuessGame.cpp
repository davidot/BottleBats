#include "GuessGame.h"
#include <sstream>
#include <charconv>
#include "../PlayerFactoryHelper.h"

namespace BBServer::Guessing {

std::string GuessPlayer::result_to_string(GuessResult result)
{
    switch (result) {
    case GuessResult::OtherPlayerFailed:
        return "incorrect";
    case GuessResult::Higher:
        return "higher";
    case GuessResult::Lower:
        return "lower";
    }

    return "?";
}

MadeGuess InteractivePlayer::guess()
{
    if (auto writer = m_communicator.output_writer(StringCommunicator::OncePerInput); writer.will_output()) {
        writer << "guess";
    }

    auto reader = m_communicator.input_reader(500);
    if (auto error = reader.has_line(); error.failed)
        return error;

    int value = -1;
    if (auto error = reader.read_int(value); error.failed)
        return error;

    return value;
}

void InteractivePlayer::guess_made(int value, GuessPlayer::GuessResult result)
{
    auto writer = m_communicator.output_writer(StringCommunicator::EveryTime);
    ASSERT(writer.will_output());
    writer << "result " << value << ' ' << result_to_string(result);
}

class IncrementingPlayer : public GuessPlayer {
public:
    static constexpr auto name = "incrementing";

    explicit IncrementingPlayer()
        : m_value(rand() % 1000) {}

    MadeGuess guess() override {
        return m_value++;
    }

private:
    int m_value {};
};


SimplePlayerCreator<GuessPlayer, IncrementingPlayer> guess_creator;

std::vector<std::string> const& GuessGame::available_algortihms() const
{
    return guess_creator.names();
}

GuessGameState<5>* GuessGame::game_for_players(std::array<std::unique_ptr<GuessPlayer>, 5> players) const
{
    int number = rand() % 1000;
    return new GuessGameState<5> {
        number,
        std::move(players)
    };
}

std::unique_ptr<GuessPlayer> GuessGame::player_from_command(std::string const& command) const
{
    return guess_creator.create(command);
}

InteractiveGameTickResult GuessGame::tick_game_state(GuessGameState<5>& game_state) const
{

    while (true) {
        PlayerIdentifier this_player = game_state.turnForPlayer;
        auto& player = game_state.players[game_state.turnForPlayer];
        auto potential_guess = player->guess();
        if (!potential_guess.has_result()) {
            std::cout << "Player " << this_player << " has no move ready waiting...\n";
            return potential_guess.to_tick_result(this_player);
        }

        int guess = potential_guess.result();

        if (guess == game_state.number) {
            std::cout << "Win for player " << this_player;
            break;
        }

        GuessPlayer::GuessResult result = guess < game_state.number ? GuessPlayer::GuessResult::Higher : GuessPlayer::GuessResult::Lower;
        player->guess_made(guess, result);

        for (size_t i = 0; i < game_state.players.size(); ++i) {
            if (i == this_player)
                continue;
            game_state.players[i]->guess_made(guess, GuessPlayer::GuessResult::OtherPlayerFailed);
        }

        game_state.turnForPlayer = (game_state.turnForPlayer + 1) % game_state.players.size();
    }

    return {};
};
}
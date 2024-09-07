#include "GuessGame.h"
#include <sstream>
#include <charconv>

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

std::optional<int> InteractivePlayer::guess()
{
    if (!sent_output) {
        sent_output = true;
        m_output_buffer.push_back("guess");
    }

    std::cout << "Guessing for interactive: Have\n"
              << m_input_buffer << '\n';
    std::cout << "Current output buffer:\n"
              << m_output_buffer.size() << " messages\n";
    auto end_of_line = m_input_buffer.find('\n');
    if (end_of_line == std::string::npos)
        return std::nullopt;

    int value = -1;
    auto result = std::from_chars(m_input_buffer.data(), m_input_buffer.data() + end_of_line, value);
    if (result.ec != std::errc {})
        return std::nullopt; // INVALID!! STOP HERE!

    sent_output = false;
    m_input_buffer.erase(0, end_of_line + 1);

    return value;
}

void InteractivePlayer::guess_made(int value, GuessPlayer::GuessResult result)
{
    std::ostringstream message {};
    message << "result " << value << ' ' << result_to_string(result);
    m_output_buffer.emplace_back(message.str());
}

class IncrementingPlayer : public GuessPlayer {
public:
    static constexpr auto name = "incrementing";

    explicit IncrementingPlayer()
        : m_value(rand() % 1000) {}

    std::optional<int> guess() override {
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

std::optional<PlayerIdentifier> GuessGame::tick_game_state(GuessGameState<5>& game_state) const
{

    while (true) {
        PlayerIdentifier this_player = game_state.turnForPlayer;
        auto& player = game_state.players[game_state.turnForPlayer];
        auto potential_guess = player->guess();
        if (!potential_guess.has_value()) {
            std::cout << "Player " << this_player << " has no move ready waiting...\n";
            return this_player;
        }

        int guess = *potential_guess;

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
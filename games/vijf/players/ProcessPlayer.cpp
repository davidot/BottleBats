#include <iostream>
#include <sstream>
#include "ProcessPlayer.h"

namespace Vijf {

ProcessPlayer::ProcessPlayer(std::vector<std::string> command)
{
    ASSERT(!command.empty());
    m_command = command;
    auto proc_or_fail = util::SubProcess::create(move(command));
    if (!proc_or_fail)
        return;

    auto& temp_process = *proc_or_fail;
    std::size_t timeTaken = 0;
    auto ready_response = temp_process.sendAndWaitForResponse("game 0 vijf\n", 1500, &timeTaken);
    if (!ready_response.has_value()) {
        std::cout << "Started to slow or did not output anything in response to: 'game 0 vijf' for " << m_command[0] << ' ' << m_command[m_command.size() - 1] << '\n';
        return;
    }
    //        if (timeTaken > 100)
    //            std::cout << "WARN: Slow startup took " << timeTaken << " ms for " << m_command[0] << ' ' << m_command[m_command.size() - 1] << '\n';

    if (*ready_response != "ready\n") {
        std::cout << "Did not get correct ready response, got _" << *ready_response << "_ from " << m_command[0] << ' ' << m_command[m_command.size() - 1] << '\n';
        return;
    }

    m_process = std::move(proc_or_fail);
}

CardNumber ProcessPlayer::take_turn(GameState const& game_state, std::size_t your_position)
{
    //        std::cout << "Process player taking turn now!\n";
    if (m_process) {
        auto result_or_failed = play_turn(game_state, your_position);
        if (result_or_failed.has_value())
            return result_or_failed.value();
        m_process->writeToWithTimeout("died\n", 10);
        m_process.reset();

        std::cerr << "Process player falling back on random player!\n";
    }
    auto& hand = *game_state.hands[your_position];
    return random_pick(hand);
}

std::optional<CardNumber> ProcessPlayer::play_turn(GameState const& game_state, std::size_t your_position)
{
    assert(m_process);

    std::ostringstream message {};
    message << "turn " << (player_count + 2) << ' ' << your_position << ' ' << game_state.players_alive << ' ' << game_state.round_number << '\n';
    for (auto const& hand : game_state.hands) {
        hand->to_sstream(message);
        message << '\n';
    }

    game_state.discarded_cards.to_sstream(message);
    message << '\n';

    game_state.deck.to_card_stack().to_sstream(message);
    message << '\n';

    std::size_t timeTaken = 0;
    auto val = std::move(*message.rdbuf()).str();
    auto result = m_process->sendAndWaitForResponse(val, 100, &timeTaken);
    if (!result.has_value()) {
        std::cerr << "ProcessPlayer '" << m_command[0] << ' ' << m_command[m_command.size() - 1] << "' sent no response!\n";
        return std::nullopt;
    }

    //        if (timeTaken > 5)
    //            std::cout << "Slow response took " << timeTaken << " ms for " << m_command[0] << ' ' << m_command[m_command.size() - 1] << '\n';

    std::string_view view = *result;
    assert(view[view.length() - 1] == '\n');
    view.remove_suffix(1);

    if (view.find("play ") != 0 || view.size() < 6) {
        std::cout << "Player response from '" << m_command[0] << ' ' << m_command[m_command.size() - 1] << "'does not start with 'play ' or there is nothing after the space. Got _" << view << "_\n";
        std::cout << "When given: " << val;
        return std::nullopt;
    }

    char card_character = view[5];
    for (auto const& card : all_cards) {
        if (card.char_representation == card_character) {
            //                std::cerr << "Decoded process player card '" << card_character << "'\n";
            return card.card;
        }
    }

    std::cout << "Player response does not map to a card got '" << card_character << "' which is not known.\n";
    return std::nullopt;
}

ProcessPlayer::~ProcessPlayer()
{
    if (m_process)
        m_process->writeToWithTimeout("died\n", 10);
}
}

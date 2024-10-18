#pragma once

#include "./InteractiveGame.h"
#include <memory>
#include <iostream>

namespace BBServer {

template<typename GameState, uint32_t NumberOfPlayers, typename BasePlayerType, typename StringBasedPlayer>
class MultiplayerGame : public InteractiveGame {
    static_assert(NumberOfPlayers > 0);

    virtual uint32_t get_num_players() const final {
        return NumberOfPlayers;
    };

    virtual InteractiveGameState* start_game(std::vector<PlayerFactory> const& setup) const final {
        ASSERT(setup.size() == NumberOfPlayers);
        std::array<std::unique_ptr<BasePlayerType>, NumberOfPlayers> players;

        for (size_t i = 0; i < NumberOfPlayers; ++i) {
            auto& command = setup[i];
            if (command.is_interactive()) {
                players[i] = command.construct_interactive<StringBasedPlayer>();
                ASSERT(players[i]);
            } else {
                players[i] = player_from_command(command.command());
                if (!players[i]) {
                    std::cout << "Failed to setup player with command: " << command.command() << '\n';
                    return nullptr;
                }
            }
        }

        return game_for_players(std::move(players));
    }

    virtual std::unique_ptr<BasePlayerType> player_from_command(std::string const& command) const = 0;

    virtual GameState* game_for_players(std::array<std::unique_ptr<BasePlayerType>, NumberOfPlayers> players) const = 0;

    virtual InteractiveGameTickResult tick_interactive_game(InteractiveGameState* game_data) const final {
        ASSERT(dynamic_cast<GameState*>(game_data));
        return tick_game_state(*static_cast<GameState*>(game_data));
    }

    virtual InteractiveGameTickResult tick_game_state(GameState& state) const = 0;
};

}


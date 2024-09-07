#pragma once

#include "../MultiplayerGame.h"
#include "../PlayerFactoryHelper.h"
#include <cstdint>
#include <memory>
#include <array>
#include <iostream>

namespace BBServer::Guessing {

class GuessPlayer {
public:
    enum class GuessResult {
        OtherPlayerFailed,
        Higher,
        Lower
    };

    static std::string result_to_string(GuessResult result);

    virtual void guess_made(int, GuessResult) {};
    virtual std::optional<int> guess() = 0;
    virtual ~GuessPlayer() {}
};

class InteractivePlayer: public GuessPlayer {
public:
    InteractivePlayer(std::string& input, std::vector<std::string>& output) :
        m_input_buffer(input), m_output_buffer(output) {}

    void guess_made(int value, GuessPlayer::GuessResult result) override;

    std::optional<int> guess() override;

private:
    bool sent_output = false;
    std::string& m_input_buffer;
    std::vector<std::string>& m_output_buffer;
};


template<uint32_t NumPlayers>
struct GuessGameState : public InteractiveGameState {
   int number = -1;

   size_t turnForPlayer = 0;
   std::array<std::unique_ptr<GuessPlayer>, NumPlayers> players {};

   explicit GuessGameState(int value, std::array<std::unique_ptr<GuessPlayer>, NumPlayers> new_players)
       : number(value)
       , players(std::move(new_players))
   {
   }

   virtual ~GuessGameState() { }
};

struct GuessGame final : public MultiplayerGame<GuessGameState<5>, 5, GuessPlayer, InteractivePlayer> {

    std::vector<std::string> const& available_algortihms() const override;

    GuessGameState<5>* game_for_players(std::array<std::unique_ptr<GuessPlayer>, 5> players) const override;

    std::unique_ptr<GuessPlayer> player_from_command(std::string const& command) const override;

    std::optional<PlayerIdentifier> tick_game_state(GuessGameState<5>& game_state) const override;
};

}

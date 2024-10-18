#pragma once

#include "../MultiplayerGame.h"
#include <cstdint>
#include <memory>
#include <array>

namespace BBServer::Guessing {

using MadeGuess = ContinuableResult<int>;

class GuessPlayer {
public:
    enum class GuessResult {
        OtherPlayerFailed,
        Higher,
        Lower
    };

    static std::string result_to_string(GuessResult result);

    virtual void guess_made(int, GuessResult) {};
    virtual MadeGuess guess() = 0;
    virtual ~GuessPlayer() {}
};

class InteractivePlayer: public GuessPlayer {
public:
    explicit InteractivePlayer(StringCommunicator communicator) :
        m_communicator(std::move(communicator)) {}

    void guess_made(int value, GuessPlayer::GuessResult result) override;

    MadeGuess guess() override;

private:
    bool sent_output = false;
    StringCommunicator m_communicator;
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

    InteractiveGameTickResult tick_game_state(GuessGameState<5>& game_state) const override;
};

}

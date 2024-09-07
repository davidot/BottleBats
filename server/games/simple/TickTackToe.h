#pragma once

#include "../MultiplayerGame.h"
#include <memory>
#include <array>

namespace BBServer::TickTackToe {


class TTTPlayer {
public:
    enum class FieldValue: uint8_t {
        Empty,
        Cross,
        Circle
    };

    static char to_char(FieldValue value);

    virtual std::optional<size_t> play(std::array<FieldValue, 9> const& field, FieldValue you) = 0;
    virtual ~TTTPlayer() {}
};


class InteractiveTTTPlayer: public TTTPlayer {
public:

    InteractiveTTTPlayer(std::string& input, std::vector<std::string>& output) :
        m_input_buffer(input), m_output_buffer(output) {}

    std::optional<size_t> play(std::array<FieldValue, 9> const& field, FieldValue you) override;

private:
    bool sent_output = false;
    std::string& m_input_buffer;
    std::vector<std::string>& m_output_buffer;
};

struct TTTGameState : public InteractiveGameState {
    using FieldValue = TTTPlayer::FieldValue;

    std::array<FieldValue, 2> player_mapping{};
    std::array<FieldValue, 9> field{};
    PlayerIdentifier turnForPlayer = 0;

    std::array<std::unique_ptr<TTTPlayer>, 2> players{};

    explicit TTTGameState(std::array<std::unique_ptr<TTTPlayer>, 2> new_players, FieldValue starting_player)
        : player_mapping({starting_player, invert(starting_player)}), players(std::move(new_players))
    {
        field.fill(FieldValue::Empty);
    }

    static FieldValue invert(FieldValue value);

    bool win_for(FieldValue symbol) const;

private:
    bool triple_for(FieldValue symbol) const;
};


struct TTTGame final : public MultiplayerGame<TTTGameState, 2, TTTPlayer, InteractiveTTTPlayer> {

    virtual std::vector<std::string> const& available_algortihms() const override;

    virtual std::unique_ptr<TTTPlayer> player_from_command(std::string const& command) const override;

    virtual TTTGameState* game_for_players(std::array<std::unique_ptr<TTTPlayer>, 2> players) const override;

    std::optional<PlayerIdentifier> tick_game_state(TTTGameState& game_state) const override;
};

}

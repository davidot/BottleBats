#pragma once

#include "../MultiplayerGame.h"
#include <array>

namespace BBServer::Connect4 {

constexpr size_t RowSize = 7;
constexpr size_t NumRows = 6;
constexpr size_t FieldSize = RowSize * NumRows;

class Connect4Player {
public:
    enum class FieldValue: uint8_t {
        Empty = 0,
        Cross,
        Circle
    };

    static char to_char(FieldValue value);

    virtual std::optional<size_t> play(std::array<FieldValue, FieldSize> const& field, FieldValue you) = 0;
    virtual ~Connect4Player() {}
};

class InteractiveConnect4Player: public Connect4Player {
public:

    InteractiveConnect4Player(std::string& input, std::vector<std::string>& output) :
        m_input_buffer(input), m_output_buffer(output) {}

    std::optional<size_t> play(std::array<FieldValue, FieldSize> const& field, FieldValue you) override;

private:
    bool sent_output = false;
    std::string& m_input_buffer;
    std::vector<std::string>& m_output_buffer;
};

struct Connect4GameState : public InteractiveGameState {
    using FieldValue = Connect4Player::FieldValue;

    std::array<FieldValue, 2> player_mapping{};
    std::array<FieldValue, FieldSize> field{};
    PlayerIdentifier turnForPlayer = 0;

    std::array<std::unique_ptr<Connect4Player>, 2> players{};

    explicit Connect4GameState(std::array<std::unique_ptr<Connect4Player>, 2> new_players, FieldValue starting_player)
        : player_mapping({starting_player, invert(starting_player)}), players(std::move(new_players))
    {
        field.fill(FieldValue::Empty);
    }

    static FieldValue invert(FieldValue value);

    bool win_for(FieldValue symbol) const;

    void drop(size_t column, FieldValue value);

private:
    bool quad_for(FieldValue symbol) const;
};


struct Connect4Game final : public MultiplayerGame<Connect4GameState, 2, Connect4Player, InteractiveConnect4Player> {

    virtual std::vector<std::string> const& available_algortihms() const override;

    virtual std::unique_ptr<Connect4Player> player_from_command(std::string const& command) const override;

    virtual Connect4GameState* game_for_players(std::array<std::unique_ptr<Connect4Player>, 2> players) const override;

    std::optional<PlayerIdentifier> tick_game_state(Connect4GameState& game_state) const override;
};



}
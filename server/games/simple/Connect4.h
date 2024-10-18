#pragma once

#include "../MultiplayerGame.h"
#include <array>
#include <cstdint>
#include <limits>

namespace BBServer::Connect4 {

constexpr uint32_t RowSize = 7;
constexpr uint32_t NumRows = 6;
constexpr uint32_t FieldSize = RowSize * NumRows;

using Connect4Move = ContinuableResult<uint32_t>;

class Connect4Player {
public:
    enum class FieldValue: uint8_t {
        Empty = 0,
        Cross,
        Circle
    };

    static char to_char(FieldValue value);

    virtual Connect4Move play(std::array<FieldValue, FieldSize> const& field, FieldValue you) = 0;
    virtual ~Connect4Player() {}
};

class InteractiveConnect4Player: public Connect4Player {
public:

    explicit InteractiveConnect4Player(StringCommunicator communicator) :
        m_communicator(std::move(communicator)) {}

    Connect4Move play(std::array<FieldValue, FieldSize> const& field, FieldValue you) override;

private:
    StringCommunicator m_communicator;
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

    InteractiveGameTickResult tick_game_state(Connect4GameState& game_state) const override;
};

}
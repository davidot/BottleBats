#pragma once

#include "../MultiplayerGame.h"
#include <array>
#include <bitset>
#include <cstdint>
#include <memory>
#include <span>
#include <sys/types.h>

namespace BBServer::Blokus {

constexpr size_t num_pieces = 21;

struct Piece {
    char letter;
    uint8_t width;
    uint8_t height;
    std::span<uint8_t> on_off;
};

// Move to cpp file
constexpr const std::array<Piece, num_pieces> ALL_PIECES = {
    Piece {},
    Piece {},
};

size_t piece_letter_to_index(char c);

struct BlokusMoveBase {
    uint8_t rotation;
    uint8_t top;
    uint8_t left;
    size_t piece_index;
};

enum class PlaceResult {
    Placed,
    PieceNotAvailable,
    NotInRightSpot, // (Also non diagonal)
    OverlapsExisting,
    TouchesSameColor,
    InvalidPlace, // top + height or left + width outside or rotation incorrect
    CanPlace,
};

struct PossibleSpots {
};

template<size_t Size, size_t NumPlayers>
struct Board {
    static_assert(Size < 32, "Size can be at most 31");

    static constexpr size_t BoardSize = Size;
    static constexpr size_t NPlayers = NumPlayers;

    using RowType = uint32_t;

    Board() {
        pieces_left.set();
    }

    struct BitBoard {
        std::array<RowType, Size> board {};
        static constexpr RowType ROW_MASK = (1 << Size) - 1;

        BitBoard()
        {
            board.fill(0);
        }

        BitBoard(Piece const& piece, uint8_t rotation, uint8_t top, uint8_t left)
        {
            // FIXME: Implement
        }

        BitBoard diagonals() const
        {
            // FIXME: Implement
            return {};
        }

        BitBoard cardinals() const
        {
            // FIXME: Implement
            return {};
        }

        bool overlaps_with(BitBoard const& other) const
        {
            // FIXME: Implement
            return false;
        }

        BitBoard& operator |=(BitBoard const& other) {
            for (size_t i = 0; i < Size; ++i) {
                board[i] |= other.board[i];
            }
        }
    };

    bool piece_available(uint8_t player, size_t piece_index) const
    {
        return pieces_left.test(_piece_available_index(player, piece_index));
    }

    PlaceResult place_piece(uint8_t piece_index, uint8_t player, uint8_t rotation, uint8_t top, uint8_t left)
    {
        if (piece_index >= ALL_PIECES.size() || !piece_available(player, piece_index))
            return PlaceResult::PieceNotAvailable;

        auto const& piece = ALL_PIECES[piece_index];

        if (!_position_valid(piece, player, rotation, top, left))
            return PlaceResult::InvalidPlace;

        BitBoard piece_board {piece, rotation, top, left};

        if (!_piece_touches_on_diagonal(piece_board, player))
            return PlaceResult::NotInRightSpot;

        if (_piece_touches_on_cardinal(piece_board, player))
            return PlaceResult::TouchesSameColor;

        if (_piece_overlaps(piece_board))
            return PlaceResult::OverlapsExisting;

        board_per_player[player] |= piece_board;
        pieces_left.set(_piece_available_index(player, piece_index), false);

        return PlaceResult::Placed;
    }

    // This does _not_ check wether the piece is available!
    PlaceResult piece_fits(Piece const& piece, uint8_t player, uint8_t rotation, uint8_t top, uint8_t left) const
    {

        if (!_position_valid(piece, player, rotation, top, left))
            return PlaceResult::InvalidPlace;

        BitBoard piece_board {piece, rotation, top, left};

        if (!_piece_touches_on_diagonal(piece_board, player))
            return PlaceResult::NotInRightSpot;

        if (_piece_touches_on_cardinal(piece_board, player))
            return PlaceResult::TouchesSameColor;

        if (_piece_overlaps(piece_board))
            return PlaceResult::OverlapsExisting;

        return PlaceResult::CanPlace;
    }

private:

    static size_t _piece_available_index(uint8_t player, uint8_t piece_index) {
        return player * ALL_PIECES.size() + piece_index;
    }

    bool _position_valid(Piece const& piece, uint8_t player, uint8_t rotation, uint8_t top, uint8_t left) const
    {
        return ((player < NumPlayers) || (rotation < 8) || (left + piece.width) <= Size || (top + piece.height) <= Size);
    }

    bool _piece_touches_on_diagonal(BitBoard const& piece_board, uint8_t player) const
    {
        auto diagonal_board = piece_board.diagonals();
        return board_per_player[player].overlaps_with(diagonal_board);
    }

    bool _piece_touches_on_cardinal(BitBoard const& piece_board, uint8_t player) const
    {
        auto cardinals_board = piece_board.cardinals();
        return board_per_player[player].overlaps_with(cardinals_board);
    }

    bool _piece_overlaps(BitBoard const& piece_board) const
    {
        for (BitBoard const& per_player : board_per_player) {
            if (per_player.overlaps_with(piece_board))
                return true;
        }
        return false;
    }

    std::array<BitBoard, NumPlayers> board_per_player{};
    std::bitset<NumPlayers * ALL_PIECES.size()> pieces_left;
};

using FourPlayerBoard = Board<20, 4>;
struct S {
    FourPlayerBoard b;
};

using BlokusMove = ContinuableResult<BlokusMoveBase>;

struct AnyBoard {
    using Blokus2Player = Board<2, 14>;
    using Blokus4Player = Board<4, 20>;

    size_t board_size() const {
        switch(board_value.index()) {
            case 0:
                return Blokus2Player::BoardSize;
            case 1:
                return Blokus4Player::BoardSize;
        }
        ASSERT_NOT_REACHED();
    }

    size_t num_players() const {
        switch(board_value.index()) {
            case 0:
                return Blokus2Player::NPlayers;
            case 1:
                return Blokus4Player::NPlayers;
        }
        ASSERT_NOT_REACHED();
    }

    bool piece_available(uint8_t player, size_t piece_index) const
    {
        ASSERT(player < num_players());
        return std::visit(
            [&](auto const& b) { return b.piece_available(player, piece_index); },
            board_value);
    }

    PlaceResult place_piece(uint8_t piece_index, uint8_t player, uint8_t rotation, uint8_t top, uint8_t left)
    {
        return std::visit(
            [&](auto& b) { return b.place_piece(piece_index, player, rotation, top, left);},
            board_value);
    }

    // This does _not_ check wether the piece is available!
    PlaceResult piece_fits(Piece const& piece, uint8_t player, uint8_t rotation, uint8_t top, uint8_t left) const
    {
        return std::visit(
            [&](auto const& b) { return b.piece_fits(piece, player, rotation, top, left);},
            board_value);
    }

    explicit AnyBoard(size_t n_players)
    {
        switch(n_players) {
            case Blokus2Player::NPlayers:
                board_value = Blokus2Player{};
            case Blokus4Player::NPlayers:
                board_value = Blokus4Player{};
            default:
                ASSERT_NOT_REACHED();
        }
    }

private:
    std::variant<Blokus2Player, Blokus4Player> board_value;
};

struct BoardAtMove {
    AnyBoard const& board;
    uint8_t const you_player;

    bool has_move(bool is_initial_move);

    BoardAtMove(AnyBoard& _board, uint8_t player_turn)
        : board(_board),
          you_player(player_turn)
    {}

private:
    // Put caching things here!
    std::vector<BlokusMove> m_possible_moves{};
};

class BlokusPlayer {
public:
    virtual BlokusMove play(BoardAtMove) = 0;
    virtual ~BlokusPlayer() { }
};

class InteractiveBlokusPlayer : public BlokusPlayer {
public:
    explicit InteractiveBlokusPlayer(StringCommunicator communicator)
        : m_communicator(std::move(communicator))
    {
    }

    virtual BlokusMove play(BoardAtMove) override;

private:
    StringCommunicator m_communicator;
};

enum class PlayerState {
    InitialTurn,
    Playing,
    Passed
};

template<size_t NumPlayers>
struct BlokusState : public InteractiveGameState {

    uint8_t turnForPlayer = 0;
    AnyBoard board;

    std::array<PlayerState, NumPlayers> player_state{};
    std::array<std::unique_ptr<BlokusPlayer>, NumPlayers> players;

    explicit BlokusState(std::array<std::unique_ptr<BlokusPlayer>, NumPlayers> new_players)
        : board(NumPlayers), players(std::move(new_players))
    {
        player_state.fill(PlayerState::InitialTurn);
    }
};

template<size_t NumPlayers>
struct BlokusGame final : public MultiplayerGame<BlokusState<NumPlayers>, NumPlayers, BlokusPlayer, InteractiveBlokusPlayer> {

    virtual std::vector<std::string> const& available_algortihms() const override
    {
        return available_algortihms();
    }

    virtual std::unique_ptr<BlokusPlayer> player_from_command(std::string const& command) const override
    {
        return player_from_command(command);
    }

    virtual BlokusState<NumPlayers>* game_for_players(std::array<std::unique_ptr<BlokusPlayer>, NumPlayers> players) const override
    {
        return new BlokusState<NumPlayers> {
            std::move(players),
        };
    }

    InteractiveGameTickResult tick_game_state(BlokusState<NumPlayers>& game_state) const override
    {
        return tick_blokus_game(game_state.board, game_state.turnForPlayer, game_state.players);
    }
};

std::vector<std::string> const& available_algortihms();
std::unique_ptr<BlokusPlayer> player_from_command(std::string const& command);
InteractiveGameTickResult tick_game_state(AnyBoard&, uint8_t& turn, std::span<PlayerState>, std::span<std::unique_ptr<BlokusPlayer>>);

}
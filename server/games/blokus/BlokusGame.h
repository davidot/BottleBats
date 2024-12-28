#pragma once

#include "../MultiplayerGame.h"
#include <array>
#include <bitset>
#include <cstdint>
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

size_t piece_letter_to_index(char c)
{
    for (size_t i = 0; i < ALL_PIECES.size(); ++i) {
        if (ALL_PIECES[i].letter == c)
            return i;
    }
    return ALL_PIECES.size();
}

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
    PieceFits,
};

struct PossibleSpots {
};

template<size_t Size, size_t NumPlayers>
struct Board {
    static_assert(Size < 32, "Size can be at most 31");

    using RowType = uint32_t;

    struct BitBoard {
        std::array<RowType, Size> board {};
        static constexpr RowType ROW_MASK = (1 << Size) - 1;

        BitBoard()
        {
            // zero initialize
        }

        BitBoard(Piece const& piece, uint8_t rotation, uint8_t top, uint8_t left)
        {
            // FIXME: Implement
        }

        BitBoard diagonals()
        {
            // FIXME: Implement
            return {};
        }

        BitBoard cardinals()
        {
            // FIXME: Implement
            return {};
        }
    };

    static constexpr size_t u64PerBoard = (Size * Size + 63) / 64;
    static constexpr size_t BitBoardSize = u64PerBoard * 64;

    bool piece_available(uint8_t player, size_t piece_index) const
    {
        return pieces_left.test(player * ALL_PIECES.size() + piece_index);
    }

    PlaceResult place_piece(uint8_t piece_index, uint8_t player, uint8_t rotation, uint8_t top, uint8_t left)
    {
        if (!piece_available(player, piece_index))
            return PlaceResult::PieceNotAvailable;

        auto const& piece = ALL_PIECES[piece_index];

        BitBoard piece_board(piece, rotation, top, left);
        auto fits = piece_fits(piece, player, rotation, top, left, &piece_board);
        if (fits != PlaceResult::PieceFits) {
            ASSERT((fits != PlaceResult::Placed));
            return fits;
        }

        board_per_player[player] |= piece_board;
    }

    // This does _not_ check wether the piece is available!
    PlaceResult piece_fits(Piece const& piece, uint8_t player, uint8_t rotation, uint8_t top, uint8_t left) const
    {

        // return _piece_fits(piece, player, rotation, top, left, nullptr);
        return {};
    }

private:
    PlaceResult _position_valid(Piece const& piece, uint8_t rotation, uint8_t top, uint8_t left) const
    {
        if ((rotation >= 8) || (left + piece.width) > Size || (top + piece.height) > Size)
            return PlaceResult::InvalidPlace;

        return PlaceResult::PieceFits;
    }

    PlaceResult _piece_touches_on_diagonal(BitBoard const& piece_board, uint8_t player) const
    {
        auto diagonal_board = piece_board.diagonal();
        if (!(board_per_player[player].overlaps_with(diagonal_board)))
            return PlaceResult::NotInRightSpot;

        return PlaceResult::PieceFits;
    }

    PlaceResult _piece_does_not_touch_on_cardinal(BitBoard const& piece_board, uint8_t player) const
    {
        auto cardinals_board = piece_board.cardinals();
        if (board_per_player[player].overlaps_with(cardinals_board))
            return PlaceResult::TouchesSameColor;

        return PlaceResult::PieceFits;
    }

    PlaceResult _piece_does_not_overlap(BitBoard const& piece_board) const
    {
        for (BitBoard const& per_player : board_per_player) {
            if (per_player.overlaps_with(piece_board))
                return PlaceResult::OverlapsExisting;
        }
        return PlaceResult::PieceFits;
    }

    std::array<BitBoard, NumPlayers> board_per_player;
    std::bitset<NumPlayers * ALL_PIECES.size()> pieces_left;
};

using FourPlayerBoard = Board<20, 4>;
struct S {
    FourPlayerBoard b;
};

constexpr uint32_t RowSize = 7;
constexpr uint32_t NumRows = 6;
constexpr uint32_t FieldSize = RowSize * NumRows;

using BlokusMove = ContinuableResult<BlokusMoveBase>;

template<size_t Size, size_t NumPlayers>
struct BoardAtMove {
    Board<Size, NumPlayers> const& board;
};

template<size_t Size, size_t NumPlayers>
class BlokusPlayer {
public:
    virtual BlokusMove play(BoardAtMove<Size, NumPlayers>) = 0;
    virtual ~BlokusPlayer() { }
};

template<size_t Size, size_t NumPlayers>
class InteractiveBlokusPlayer : public BlokusPlayer<Size, NumPlayers> {
public:
    explicit InteractiveBlokusPlayer(StringCommunicator communicator)
        : m_communicator(std::move(communicator))
    {
    }

    virtual BlokusMove play(BoardAtMove<Size, NumPlayers>) override;

private:
    StringCommunicator m_communicator;
};

template<size_t Size, size_t NumPlayers>
struct BlokusState : public InteractiveGameState {

    Board<Size, NumPlayers> board;
    size_t turnForPlayer = 0;

    std::array<std::unique_ptr<BlokusPlayer<Size, NumPlayers>>, NumPlayers> players {};

    explicit BlokusState(std::array<std::unique_ptr<BlokusPlayer<Size, NumPlayers>>, NumPlayers> new_players)
        : players(std::move(new_players))
    {
    }
};

template<size_t Size, size_t NumPlayers>
struct BlokusGame final : public MultiplayerGame<BlokusState<Size, NumPlayers>, NumPlayers, BlokusPlayer<Size, NumPlayers>, InteractiveBlokusPlayer<Size, NumPlayers>> {

    virtual std::vector<std::string> const& available_algortihms() const override;

    virtual std::unique_ptr<BlokusPlayer<Size, NumPlayers>> player_from_command(std::string const& command) const override;

    virtual BlokusState<Size, NumPlayers>* game_for_players(std::array<std::unique_ptr<BlokusPlayer<Size, NumPlayers>>, NumPlayers> players) const override;

    InteractiveGameTickResult tick_game_state(BlokusState<Size, NumPlayers>& game_state) const override;
};

}
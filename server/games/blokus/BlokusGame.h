#pragma once

#include "../MultiplayerGame.h"
#include "BlokusPieces.h"
#include <algorithm>
#include <array>
#include <bitset>
#include <cstdint>
#include <memory>
#include <span>

namespace BBServer::Blokus {

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

using RowType = uint32_t;

void add_diagonals(std::span<RowType const> input, std::span<RowType> output);
void add_cardinals(std::span<RowType const> input, std::span<RowType> output);
void add_spots(std::span<RowType const> input, std::span<RowType> output);
bool has_overlap(std::span<RowType const> lhs, std::span<RowType const> rhs);

template<size_t NumPlayers, size_t Size>
struct Board {
    static_assert(Size < 32, "Size can be at most 31");

    static constexpr size_t BoardSize = Size;
    static constexpr size_t NPlayers = NumPlayers;

    Board()
    {
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
            board.fill(0);

            auto mark_on = [&](uint32_t x, uint32_t y) {
                board[top + y] |= 1 << (x + left);
            };

            for (uint32_t y = 0; y < piece.height; ++y) {
                for (uint32_t x = 0; x < piece.width; ++x) {
                    bool on = piece.on_off[y * piece.width + x] == 1;
                    if (!on)
                        continue;
                    switch (rotation) {
                    case 0:
                        mark_on(x, y);
                        break;
                    case 1:
                        mark_on(piece.width - x - 1, y);
                        break;
                    case 2:
                        mark_on(x, piece.height - y - 1);
                        break;
                    case 3:
                        mark_on(piece.width - x - 1, piece.height - y - 1);
                        break;
                    case 4:
                        mark_on(y, x);
                        break;
                    case 5:
                        mark_on(piece.height - y - 1, x);
                        break;
                    case 6:
                        mark_on(y, piece.width - x - 1);
                        break;
                    case 7:
                        mark_on(piece.height - y - 1, piece.width - x - 1);
                        break;
                    }
                }
            }
        }

        BitBoard diagonals() const
        {
            BitBoard new_board {};
            add_diagonals(board, new_board.board);
            return new_board;
        }

        BitBoard cardinals() const
        {
            BitBoard new_board {};
            add_cardinals(board, new_board.board);
            return new_board;
        }

        bool overlaps_with(BitBoard const& other) const
        {
            return has_overlap(board, other.board);
        }

        BitBoard& operator|=(BitBoard const& other)
        {
            add_spots(other.board, board);
            return *this;
        }

        bool empty() const
        {
            return std::all_of(board.begin(), board.end(), [](RowType val) {
                return val == 0;
            });
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

        BitBoard piece_board { piece, rotation, top, left };

        if (!_first_move_for_player(player) && !_piece_touches_on_diagonal(piece_board, player))
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

        BitBoard piece_board { piece, rotation, top, left };

        if (!_first_move_for_player(player) && !_piece_touches_on_diagonal(piece_board, player))
            return PlaceResult::NotInRightSpot;

        if (_piece_touches_on_cardinal(piece_board, player))
            return PlaceResult::TouchesSameColor;

        if (_piece_overlaps(piece_board))
            return PlaceResult::OverlapsExisting;

        return PlaceResult::CanPlace;
    }

    std::span<RowType const> board_for_player(uint8_t player) const
    {
        return board_per_player[player].board;
    }

private:
    static size_t _piece_available_index(uint8_t player, uint8_t piece_index)
    {
        return player * ALL_PIECES.size() + piece_index;
    }

    bool _position_valid(Piece const& piece, uint8_t player, uint8_t rotation, uint8_t top, uint8_t left) const
    {
        uint8_t wide = rotation < 4 ? piece.width : piece.height;
        uint8_t high = rotation < 4 ? piece.height : piece.width;
        return (player < NumPlayers) && (rotation < 8) && (left + wide) <= Size && (top + high) <= Size;
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

    bool _first_move_for_player(uint8_t player) const
    {
        return board_per_player[player].empty();
    }

    std::array<BitBoard, NumPlayers> board_per_player {};
    std::bitset<NumPlayers * ALL_PIECES.size()> pieces_left;
};

using BlokusMove = ContinuableResult<BlokusMoveBase>;

struct AnyBoard {
    using Blokus2Player = Board<2, 14>;
    using Blokus4Player = Board<4, 20>;

    uint32_t board_size() const
    {
        switch (board_value.index()) {
        case 0:
            return Blokus2Player::BoardSize;
        case 1:
            return Blokus4Player::BoardSize;
        }
        ASSERT_NOT_REACHED();
    }

    uint32_t num_players() const
    {
        switch (board_value.index()) {
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
            [&](auto& b) { return b.place_piece(piece_index, player, rotation, top, left); },
            board_value);
    }

    // This does _not_ check wether the piece is available!
    PlaceResult piece_fits(Piece const& piece, uint8_t player, uint8_t rotation, uint8_t top, uint8_t left) const
    {
        return std::visit(
            [&](auto const& b) { return b.piece_fits(piece, player, rotation, top, left); },
            board_value);
    }

    // This does _not_ check wether the piece is available!
    std::span<RowType const> board_for_player(uint8_t player) const
    {
        return std::visit(
            [&](auto const& b) { return b.board_for_player(player); },
            board_value);
    }

    explicit AnyBoard(size_t n_players)
    {
        switch (n_players) {
        case Blokus2Player::NPlayers:
            board_value = Blokus2Player {};
            break;
        case Blokus4Player::NPlayers:
            board_value = Blokus4Player {};
            break;
        default:
            ASSERT_NOT_REACHED();
        }
    }

private:
    std::variant<Blokus2Player, Blokus4Player> board_value;
};

struct BoardAtMove {
    AnyBoard const& board;
    bool is_initial_move;
    uint8_t const you_player;

    BoardAtMove(AnyBoard& _board, uint8_t player_turn, bool initial_move);

    struct Location {
        uint8_t top;
        uint8_t left;
    };

    bool any_move_available() const;

    std::vector<Location> const& start_spots();

    std::vector<BlokusMoveBase> const& all_moves();

    enum class MoveMatch {
        Match,
        NoMatch
    };

    template<typename Func>
    BlokusMoveBase first_matching_move(Func&& func)
    {
        ASSERT(m_possible_moves.size() > 0);
        for (auto const& move : m_possible_moves) {
            if (func(move) == MoveMatch::Match)
                return move;
        }

        while (find_next_move()) {
            if (func(const_cast<BlokusMoveBase const&>(m_possible_moves.back())) == MoveMatch::Match)
                return m_possible_moves.back();
        }

        return {
            0,
            0,
            0,
            ALL_PIECES.size(),
        };
    }

private:
    bool find_next_move();
    void find_all_moves();
    void _find_first_move();

    std::vector<BlokusMoveBase> m_possible_moves {};
    std::vector<size_t> available_pieces {};
    // Ordered by row, then columns (i.e. top then left)
    std::vector<Location> m_start_spots {};

    size_t start_spot_index { 0 };
    size_t piece_index_index { 0 };
    size_t rotation_index { 0 };
    size_t offset_index { 0 };
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

    std::array<PlayerState, NumPlayers> player_state {};
    std::array<std::unique_ptr<BlokusPlayer>, NumPlayers> players;

    explicit BlokusState(std::array<std::unique_ptr<BlokusPlayer>, NumPlayers> new_players)
        : board(NumPlayers)
        , players(std::move(new_players))
    {
        player_state.fill(PlayerState::InitialTurn);
    }
};

std::vector<std::string> const& available_blokus_algortihms();
std::unique_ptr<BlokusPlayer> blokus_player_from_command(std::string const& command);
InteractiveGameTickResult tick_blokus_game(AnyBoard&, uint8_t& turn, std::span<PlayerState>, std::span<std::unique_ptr<BlokusPlayer>>);

template<size_t NumPlayers>
struct BlokusGame final : public MultiplayerGame<BlokusState<NumPlayers>, NumPlayers, BlokusPlayer, InteractiveBlokusPlayer> {

    virtual std::vector<std::string> const& available_algortihms() const override
    {
        return available_blokus_algortihms();
    }

    virtual std::unique_ptr<BlokusPlayer> player_from_command(std::string const& command) const override
    {
        return blokus_player_from_command(command);
    }

    virtual BlokusState<NumPlayers>* game_for_players(std::array<std::unique_ptr<BlokusPlayer>, NumPlayers> players) const override
    {
        return new BlokusState<NumPlayers> {
            std::move(players),
        };
    }

    InteractiveGameTickResult tick_game_state(BlokusState<NumPlayers>& game_state) const override
    {
        return tick_blokus_game(game_state.board, game_state.turnForPlayer, game_state.player_state, game_state.players);
    }
};

}
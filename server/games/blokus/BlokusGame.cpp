#include "BlokusGame.h"
#include "../PlayerFactoryHelper.h"
#include "BlokusPieces.h"
#include <algorithm>
#include <array>
#include <bit>
#include <bitset>
#include <cstdint>

namespace BBServer::Blokus {

uint32_t piece_letter_to_index(char c)
{
    for (uint32_t i = 0; i < ALL_PIECES.size(); ++i) {
        if (ALL_PIECES[i].letter == c)
            return i;
    }
    return ALL_PIECES.size();
}

struct RandomBlokusPlayer : public BlokusPlayer {
    static constexpr auto name = "random-move";

    BlokusMove play(BoardAtMove at_move) override
    {
        auto& moves = at_move.all_moves();
        ASSERT(moves.size() > 0);
        return moves[rand() % moves.size()];
    }
};

struct FirstBlokusPlayer : public BlokusPlayer {
    static constexpr auto name = "first1";

    BlokusMove play(BoardAtMove at_move) override
    {
        return at_move.first_matching_move([](auto const&) {
            return BoardAtMove::MoveMatch::Match;
        });
    }
};

static SimplePlayerCreator<BlokusPlayer, RandomBlokusPlayer, FirstBlokusPlayer> creator;

std::vector<std::string> const& available_blokus_algortihms()
{
    return creator.names();
}

std::unique_ptr<BlokusPlayer> blokus_player_from_command(std::string const& command)
{
    return creator.create(command);
}

void add_diagonals(std::span<RowType const> input, std::span<RowType> output)
{
    ASSERT(input.size() == output.size());
    ASSERT(input.size() >= 2);

    output[1] |= input[0] << 1;
    output[1] |= input[0] >> 1;

    for (size_t i = 1; i < input.size() - 1; ++i) {
        RowType shifted = (input[i] << 1) | (input[i] >> 1);
        output[i - 1] |= shifted;
        output[i + 1] |= shifted;
    }

    const size_t final_row = input.size() - 1;
    output[final_row - 1] |= input[final_row] << 1;
    output[final_row - 1] |= input[final_row] >> 1;
}

void add_cardinals(std::span<RowType const> input, std::span<RowType> output)
{
    ASSERT(input.size() == output.size());
    ASSERT(input.size() >= 2);

    for (size_t i = 0; i < input.size(); ++i) {
        output[i] |= (input[i] << 1);
        output[i] |= (input[i] >> 1);
        if (i > 0)
            output[i - 1] |= input[i];
        if (i + 1 < input.size())
            output[i + 1] |= input[i];
    }
}

void add_spots(std::span<RowType const> input, std::span<RowType> output)
{
    ASSERT(input.size() == output.size());
    for (size_t i = 0; i < input.size(); ++i) {
        output[i] |= input[i];
    }
}

bool has_overlap(std::span<RowType const> lhs, std::span<RowType const> rhs)
{
    ASSERT(lhs.size() == rhs.size());
    for (size_t i = 0; i < lhs.size(); ++i) {
        if ((lhs[i] & rhs[i]) != 0)
            return true;
    }
    return false;
}

template<typename Func>
void for_each_one(std::span<RowType const> input, Func&& func)
{
    uint32_t max_val = input.size();
    for (uint32_t row = 0; row < max_val; ++row) {
        RowType values = input[row];
        if (values == 0)
            continue;

        uint32_t col = 0;
        while (true) {
            uint32_t offset = std::countr_zero(values);

            col += offset + 1;
            if (col >= max_val)
                break;

            func(row, col - 1);
            values >>= (offset + 1);
        }
    }
}

InteractiveGameTickResult tick_blokus_game(AnyBoard& board, uint8_t& turn, std::span<PlayerState> player_states, std::span<std::unique_ptr<BlokusPlayer>> players)
{
    while (true) {
        auto turn_for = turn;
        turn = (turn + 1) % players.size();

        if (player_states[turn_for] == PlayerState::Passed)
            continue;

        auto& player = players[turn_for];

        BoardAtMove board_at_move { board, turn_for, player_states[turn_for] == PlayerState::InitialTurn };

        if (!board_at_move.any_move_available()) {
            std::cout << "No more moves for: " << (int)turn_for << '\n';
            player_states[turn_for] = PlayerState::Passed;
            if (std::all_of(player_states.begin(), player_states.end(), [](PlayerState state) { return state == PlayerState::Passed; })) {
                for (size_t i = 0; i < player_states.size(); ++i) {
                    std::cout << "Board for " << i << '\n';
                    for (auto& row : board.board_for_player(i)) {
                        std::bitset<14> val_bin { row };
                        std::cout << val_bin << '\n';
                    }
                }
                // FIXME: Compute score and store!
                return {};
            }
            continue;
        } else {
            player_states[turn_for] = PlayerState::Playing;
        }

        auto potential_move = player->play(std::move(board_at_move));

        if (!potential_move.has_result())
            return potential_move.to_tick_result(turn_for);

        auto& made_move = potential_move.result();
        std::cout << (int)turn_for << " playing " << ALL_PIECES[made_move.piece_index].letter << '\n';

        auto result = board.place_piece(made_move.piece_index, turn_for, made_move.rotation, made_move.top, made_move.left);
        if (result != PlaceResult::Placed)
            return { turn_for, "Wrong!" }; // FIXME: Make nicer error message
    }

    return {};
}

void BoardAtMove::_find_first_move()
{
    m_start_spots.reserve(64);
    available_pieces.reserve(ALL_PIECES.size());
    for (size_t i = 0; i < ALL_PIECES.size(); ++i) {
        if (board.piece_available(you_player, i))
            available_pieces.push_back(i);
    }

    uint32_t board_size = board.board_size();

    if (is_initial_move) {
        std::cout << "intial move for " << (int)you_player << '\n';
        // Should always have tiny piece available so as long as starting spot is fine we know what to do.
        ASSERT((ALL_PIECES[0].width == 1) && (ALL_PIECES[0].height == 1));
        ASSERT(board.piece_available(you_player, 0));

        bool two_player = board.num_players() == 2;
        if (two_player) {
            constexpr uint32_t start_spot_offset_two_player = 4;
            for (uint32_t spot : { start_spot_offset_two_player, board_size - start_spot_offset_two_player }) {
                if (you_player == 0 || board.piece_fits(A_PIECE, you_player, 0, spot, spot) == PlaceResult::CanPlace) {
                    m_start_spots.emplace_back(spot, spot);
                }
            }
        } else {
            uint8_t top = (you_player & 2) == 0 ? 0 : board_size - 1;
            uint8_t left = (you_player & 2) == 0 ? 0 : board_size - 1;
            ASSERT(board.piece_fits(A_PIECE, you_player, 0, top, left));

            m_start_spots.emplace_back(top, left);
        }

        ASSERT(m_start_spots.size() > 0);

        for (auto& loc : m_start_spots) {
            std::cout << "Got option: " << (int)loc.top << ", " << (int)loc.left << '\n';
        }

        find_next_move();
        return;
    }

    std::vector<RowType> fake_board(board_size, 0);

    // std::cout << "Raw values:\n";

    // for (auto& val : board.board_for_player(you_player)) {
    //     std::bitset<32> val_bin { val };
    //     std::cout << val_bin << '\n';
    // }

    add_diagonals(board.board_for_player(you_player), fake_board);
    for (auto& val : fake_board) {
        val = ~val;
    }

    // Now has 0s for valid spots
    // Fill in the ones which are not actually valid!
    add_cardinals(board.board_for_player(you_player), fake_board);

    for (size_t i = 0; i < board.num_players(); ++i) {
        add_spots(board.board_for_player(i), fake_board);
    }

    // Convert back to 1s at spots which are valid
    for (auto& val : fake_board) {
        val = ~val;
    }

    // std::cout << "Got values:\n";

    // for (auto& val : fake_board) {
    //     std::bitset<32> val_bin { val };
    //     std::cout << val_bin << '\n';
    // }

    for_each_one(fake_board, [&](uint32_t row, uint32_t col) {
        m_start_spots.emplace_back(row, col);
    });

    board.board_for_player(you_player);

    find_next_move();
}

bool BoardAtMove::find_next_move()
{
    while (start_spot_index < m_start_spots.size()) {
        Location const& spot = m_start_spots[start_spot_index];
        while (piece_index_index < available_pieces.size()) {
            auto piece_index = available_pieces[piece_index_index];
            Piece const& piece = ALL_PIECES[piece_index];
            while (rotation_index < piece.unique_rotations.size()) {
                uint8_t rotation = piece.unique_rotations[rotation_index];
                while (offset_index < (piece.width * piece.height)) {
                    uint32_t size = rotation < 4 ? piece.width : piece.height;
                    uint32_t left_offset = offset_index % size;
                    uint32_t top_offset = offset_index / size;
                    ++offset_index;

                    if (spot.left < left_offset || spot.top < top_offset)
                        continue;

                    if (auto res = board.piece_fits(piece,
                            you_player,
                            rotation,
                            spot.top - top_offset,
                            spot.left - left_offset);
                        res == PlaceResult::CanPlace) {
                        m_possible_moves.emplace_back(
                            rotation,
                            spot.top - top_offset,
                            spot.left - left_offset,
                            piece_index);
                        return true;
                    }
#if 0
                    else {
                        std::cout << piece.letter << "Did not fit at " << (int)spot.top - top_offset << ", " << (int)spot.left - left_offset << " with ro " << (int)rotation << '\n';
                        std::cout << "    got: " << (int)res << '\n';
                    }
#endif
                }
                offset_index = 0;
                ++rotation_index;
            }
            rotation_index = 0;
            ++piece_index_index;
        }
        piece_index_index = 0;
        ++start_spot_index;
    }
    return false;
}

void BoardAtMove::find_all_moves()
{
    while (find_next_move())
        ;
}

std::vector<BoardAtMove::Location> const& BoardAtMove::start_spots()
{
    return m_start_spots;
}
std::vector<BlokusMoveBase> const& BoardAtMove::all_moves()
{
    find_all_moves();
    return m_possible_moves;
}

BoardAtMove::BoardAtMove(AnyBoard& _board, uint8_t player_turn, bool initial_move)
    : board(_board)
    , is_initial_move(initial_move)
    , you_player(player_turn)
{
    _find_first_move();
}

bool BoardAtMove::any_move_available() const
{
    return !m_possible_moves.empty();
}

static char PLAYER_NAMES[5] {
    'R',
    'B',
    'Y',
    'G',
};

BlokusMove InteractiveBlokusPlayer::play(BoardAtMove move)
{
    if (auto writer = m_communicator.output_writer(StringCommunicator::OncePerInput); writer.will_output()) {
        writer << "turn ";
        size_t num_players = move.board.num_players();
        writer << num_players << ' ';
        auto print_pieces = [&](size_t print_player) {
            writer << PLAYER_NAMES[print_player] << ':';
            bool first = true;
            for (size_t i = 0; i < ALL_PIECES.size(); ++i) {
                if (!move.board.piece_available(print_player, i))
                    continue;

                if (!first)
                    writer << ',';

                first = false;
                writer << ALL_PIECES[i].letter;
            }
        };
        print_pieces(move.you_player);
        writer << ' ';

        for (size_t i = 0; i < num_players; ++i) {
            if (i == move.you_player)
                continue;
            print_pieces(i);
            writer << ' ';
        }

        uint32_t board_size = move.board.board_size();
        auto next_start_spot = move.start_spots().begin();
        auto end_start_spot = move.start_spots().end();

        std::vector<std::span<RowType const>> player_boards;
        player_boards.reserve(num_players);
        for (uint32_t i = 0; i < num_players; ++i) {
            player_boards.emplace_back(move.board.board_for_player(i));
        }

        for (uint32_t r = 0; r < board_size; ++r) {
            if (r > 0)
                writer << '|';
            for (uint32_t c = 0; c < board_size; ++c) {
                if (next_start_spot != end_start_spot && next_start_spot->top == r && next_start_spot->left == c) {
                    writer << 'O';
                    ++next_start_spot;
                    continue;
                }

                bool empty = true;

                for (uint32_t i = 0; i < num_players; ++i) {
                    if ((player_boards[i][r] & (1 << c)) != 0) {
                        writer << PLAYER_NAMES[i];
                        empty = false;
                        break;
                    }
                }

                if (empty)
                    writer << '-';
            }
        }
    }

    auto reader = m_communicator.input_reader(1000);
    if (auto error = reader.has_line(); error.failed)
        return error;

    std::string_view piece;

    if (auto error = reader.read_value(piece); error.failed)
        return error;

    if (piece.size() != 1)
        return reader.error(std::string("Piece name exactly one character long! Got: ") + std::string(piece));

    uint32_t piece_index = piece_letter_to_index(piece[0]);
    if (piece_index >= ALL_PIECES.size())
        return reader.error(std::string("Unknown piece: ") + std::string(piece));

    if (!move.board.piece_available(move.you_player, piece_index))
        return reader.error(std::string("Do not have piece to place: ") + piece[0]);

    uint8_t rotation;
    if (auto error = reader.read_int(rotation, uint8_t(0), uint8_t(7)); error.failed)
        return error;

    uint8_t top;
    if (auto error = reader.read_int(top, uint8_t(0), (uint8_t)(move.board.board_size() - 1u)); error.failed)
        return error;

    uint8_t left;
    if (auto error = reader.read_int(left, uint8_t(0), (uint8_t)(move.board.board_size() - 1u)); error.failed)
        return error;

    return BlokusMoveBase { rotation, top, left, piece_index };
}

}
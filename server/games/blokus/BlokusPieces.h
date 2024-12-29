#pragma once

#include <array>
#include <cstdint>
#include <numeric>
#include <vector>

namespace BBServer::Blokus {

constexpr const uint32_t NumPieces = 21;

struct Piece {
    char letter;
    uint8_t width;
    uint8_t height;
    uint8_t area;
    std::vector<uint8_t> on_off;
    std::vector<uint8_t> unique_rotations;

    Piece(char _letter, uint8_t _width, uint8_t _height, std::vector<uint8_t> _on_off, std::vector<uint8_t> _unique_rotations)
        : letter(_letter)
        , width(_width)
        , height(_height)
        , on_off(std::move(_on_off))
        , unique_rotations(std::move(_unique_rotations))
    {
        area = std::accumulate(on_off.begin(), on_off.end(), 0u);
    }
};

extern const Piece A_PIECE;
extern const std::array<const Piece, NumPieces> ALL_PIECES;

uint32_t piece_letter_to_index(char c);

}
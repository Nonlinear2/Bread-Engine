#pragma once

#include "chess.hpp"
#include <array>

using namespace chess;

namespace PSM {

constexpr int REF_VALUE = 150;

constexpr std::array<int, 64> pawn_map = {
       0,    0,    0,    0,    0,    0,    0,    0,
       0,    0,    0,    0,    0,    0,    0,    0,
      38,   60,   10,  100,  100,   10,   60,   38,
      50,   60,   40,  150,  150,   40,   60,   50,
      28,   28,   56,  112,  112,   56,   28,   28,
      56,   56,   84,  140,  140,   84,   56,   56,
     196,  196,  196,  196,  196,  196,  196,  196,
     252,  252,  252,  252,  252,  252,  252,  252,
};

constexpr std::array<int, 64> knight_map = {
     -18,    0,   18,   18,   18,   18,    0,  -18,
       0,   35,   70,   88,   88,   70,   35,    0,
      18,   88,  105,  123,  123,  105,   88,   18,
      18,   70,  123,  141,  141,  123,   70,   18,
      18,   88,  123,  141,  141,  123,   88,   18,
      18,   70,  105,  123,  123,  105,   70,   18,
       0,   35,   70,   70,   70,   70,   35,    0,
     -18,    0,   18,   18,   18,   18,    0,  -18,
};

constexpr std::array<int, 64> bishop_map = {
      65,   81,   81,   81,   81,   81,   81,   65,
      81,  114,   98,   98,   98,   98,  114,   81,
      81,  130,  130,  130,  130,  130,  130,   81,
      81,   98,  130,  130,  130,  130,   98,   81,
      81,  114,  114,  130,  130,  114,  114,   81,
      81,   98,  114,  130,  130,  114,   98,   81,
      81,   98,   98,   98,   98,   98,   98,   81,
      65,   81,   81,   81,   81,   81,   81,   65,
};

constexpr std::array<int, 64> rook_map = {
      34,   34,   34,   46,   46,   34,   34,   34,
      23,   34,   34,   34,   34,   34,   34,   23,
      23,   34,   34,   34,   34,   34,   34,   23,
      23,   34,   34,   34,   34,   34,   34,   23,
      23,   34,   34,   34,   34,   34,   34,   23,
      23,   34,   34,   34,   34,   34,   34,   23,
      63,   69,   69,   69,   69,   69,   69,   63,
      34,   34,   34,   34,   34,   34,   34,   34,
};

constexpr std::array<int, 64> queen_map = {
      20,   31,   31,   41,   41,   31,   31,   20,
      31,   52,   72,   52,   52,   52,   52,   31,
      31,   72,   72,   72,   72,   72,   52,   31,
      52,   52,   72,   72,   72,   72,   52,   41,
      41,   52,   72,   72,   72,   72,   52,   41,
      31,   52,   72,   72,   72,   72,   52,   31,
      31,   52,   52,   52,   52,   52,   52,   31,
      20,   31,   31,   41,   41,   31,   31,   20,
};

constexpr std::array<std::array<int, 64>, 5> psms = {
    pawn_map, knight_map, bishop_map, rook_map, queen_map,
};

constexpr int get_pawn_psm(Color color, Square sq){
    return color == Color::WHITE ? pawn_map[sq.index()] : pawn_map[sq.flip().index()];
}

constexpr int get_psm(Piece piece, Square sq){
    assert(piece.type() != PieceType::NONE);
    if (piece.type() == PieceType::KING)
        return 0;
    return piece.color() == Color::WHITE ? psms[piece.type()][sq.index()] : psms[piece.type()][sq.flip().index()];
}

}; // namespace PSM
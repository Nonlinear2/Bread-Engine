#pragma once

#include "chess.hpp"
#include <array>

using namespace chess;

constexpr std::array<int, 64> pawn_map = {
            0,    0,    0,    0,   0,     0,    0,    0, 
            0,    0,    0,    0,   0,     0,    0,    0, 
            28,  -28,  -56,   0,   0,    -56,  -28,   28, 
            0,    0,    0,   84,   84,    0,    0,    0, 
            28,   28,   56,  112,  112,   56,   28,   28, 
            56,   56,   84,  140,  140,   84,   56,   56, 
           196,  196,  196,  196,  196,  196,  196,  196, 
           252,  252,  252,  252,  252,  252,  252,  252};

constexpr std::array<int, 64> king_map_mg = { // middle game
            2,   3,   2,   0,   0,   2,   3,   2, 
            2,   2,   0,   0,   0,   0,   2,   2, 
           -0,  -1,  -1,  -1,  -1,  -1,  -1,  -0, 
           -1,  -1,  -1,  -2,  -2,  -1,  -1,  -1, 
           -1,  -2,  -2,  -2,  -2,  -2,  -2,  -1, 
           -1,  -2,  -2,  -2,  -2,  -2,  -2,  -1, 
           -1,  -2,  -2,  -2,  -2,  -2,  -2,  -1, 
           -1,  -2,  -2,  -2,  -2,  -2,  -2,  -1};

constexpr std::array<int, 64> king_map_eg = { // end game
           -13,  -7,  -7,  -7,  -7,  -7,  -7,  -13, 
           -7,  -7,   3,   3,   3,   3,  -7,  -7, 
           -7,   0,   10,   13,   13,   10,   0,  -7, 
           -7,   0,   13,   16,   16,   13,   0,  -7, 
           -7,   0,   13,   16,   16,   13,   0,  -7, 
           -7,   0,   10,   13,   13,   10,   0,  -7, 
           -7,  -3,   0,   3,   3,   0,  -3,  -7, 
           -13,  -10,  -7,  -3,  -3,  -7,  -10,  -13};

struct PieceSquareMaps {
    std::array<std::array<int, 64>, 2> all_ksm = {king_map_mg, king_map_eg};

    constexpr int get_pawn_psm(Color color, Square sq) const {
        return color == Color::WHITE ? pawn_map[sq.index()] : pawn_map[sq.flip().index()];
    }

    constexpr int get_ksm(Piece king, bool is_endgame, Square from_sq, Square to_sq) const {
        if (king.color() == Color::BLACK){
            from_sq = from_sq.flip();
            to_sq = to_sq.flip();
        }
        return all_ksm[is_endgame][to_sq.index()] - all_ksm[is_endgame][from_sq.index()];
    }
};
#pragma once

#include "chess.hpp"
#include <array>

using namespace chess;

namespace PSM {

constexpr std::array<int, 64> pawn_map = {
            0,    0,    0,    0,   0,     0,    0,    0, 
            0,    0,    0,    0,   0,     0,    0,    0, 
            28,  -28,  -56,   0,   0,    -56,  -28,   28, 
            0,    0,    0,   84,   84,    0,    0,    0, 
            28,   28,   56,  112,  112,   56,   28,   28, 
            56,   56,   84,  140,  140,   84,   56,   56, 
           196,  196,  196,  196,  196,  196,  196,  196, 
           252,  252,  252,  252,  252,  252,  252,  252};

constexpr int get_pawn_psm(Color color, Square sq){
    return color == Color::WHITE ? pawn_map[sq.index()] : pawn_map[sq.flip().index()];
}

}; // namespace PSM
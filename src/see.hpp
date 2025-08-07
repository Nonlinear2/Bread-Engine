#pragma once

#include "constants.hpp"
#include "misc.hpp"
#include "chess.hpp"

using namespace chess;


namespace SEE {

bool evaluate(const Board& board, Move move, int threshold);
PieceType pop_lva(const Board& board, Bitboard& occupied, const Bitboard& attackers, Color color);

} // namespace SEE
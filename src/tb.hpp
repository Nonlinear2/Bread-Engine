#pragma once 

#include "chess.hpp"
#include "tbprobe.hpp"
#include "constants.hpp"

using namespace chess;

namespace TB {

bool probe_wdl(Board& pos, int& eval);

bool probe_root_dtz(Board& pos, Move& move, Movelist& moves, bool generate_moves);

Move tb_result_to_move(unsigned int tb_result);

} // namespace TB


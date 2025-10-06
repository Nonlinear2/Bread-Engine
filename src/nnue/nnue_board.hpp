#pragma once 

#include <stack>

#include "chess.hpp"
#include "tbprobe.hpp"
#include "piece_square_tables.hpp"
#include "nnue.hpp"
#include "misc.hpp"

class NnueBoard: public Board {
    public:
    NNUE nnue_ = NNUE();

    NnueBoard();
    NnueBoard(std::string_view fen);
    
    void synchronize();

    void update_state(Move move);

    void restore_state(Move move);

    int evaluate();

    bool try_outcome_eval(int& eval);

    bool probe_wdl(int& eval);

    bool probe_root_dtz(Move& move, Movelist& moves, bool generate_moves);

    Move tb_result_to_move(unsigned int tb_result);
    
    std::pair<std::vector<int>, std::vector<int>> get_features();

    private:
    AccumulatorsStack accumulators_stack;

    modified_features get_modified_features(Move move, Color color);

    bool is_updatable_move(Move move);
};
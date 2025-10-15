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

    std::pair<std::vector<int>, std::vector<int>> get_features();

    AccumulatorsStack accumulators_stack;
    private:

    modified_features get_modified_features(Move move, Color color);

    bool is_updatable_move(Move move);
};
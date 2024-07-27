#pragma once 

#include <stack>

#include "chess.hpp"
#include "tbprobe.hpp"
#include "piece_square_tables.hpp"
#include "nnue.hpp"
#include "misc.hpp"

class NnueBoard: public chess::Board {
    public:
    NNUE nnue_ = NNUE(bread_NNUE_MODEL_PATH);

    NnueBoard();
    NnueBoard(std::string_view fen);
    
    void synchronize();

    bool last_move_null();

    void update_state(chess::Move move);

    void restore_state(chess::Move move);

    int evaluate();

    bool try_outcome_eval(int& eval);

    bool probe_wdl(int& eval);

    bool probe_dtz(chess::Move& move);

    private:
    std::stack<Accumulator> accumulator_stack;

    std::vector<int> get_HKP(bool color);

    modified_features get_modified_features(chess::Move move, bool color);

    bool is_updatable_move(chess::Move move);
};
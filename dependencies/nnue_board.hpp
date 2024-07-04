#pragma once 

#include <stack>

#include "chess.hpp"
#include "tbprobe.hpp"
#include "piece_square_tables.hpp"
#include "nnue.hpp"

class NnueBoard: public chess::Board {
    public:
    NNUE nnue_ = NNUE(bread_NNUE_MODEL_PATH);

    std::stack<Accumulator> accumulator_stack;

    NnueBoard();
    NnueBoard(std::string_view fen);
    
    void synchronize();

    std::vector<int> get_HKP(bool color);

    modified_features get_modified_features(chess::Move move, bool color);

    void update_state(chess::Move move);

    void restore_state(chess::Move move);

    bool is_basic_move(chess::Move move);
    
    float evaluate();

    bool probe_wdl(float& eval);

    bool probe_dtz(chess::Move& move);
};
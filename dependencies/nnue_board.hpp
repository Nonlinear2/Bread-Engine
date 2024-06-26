#pragma once 

#include <stack>

#include "search_board.hpp"
#include "piece_square_tables.hpp"
#include "nnue.hpp"

class NnueBoard: public SearchBoard {
    public:
    NNUE nnue_ = NNUE(bread_NNUE_MODEL_PATH);

    std::stack<Accumulator> accumulator_stack;

    NnueBoard();
    NnueBoard(std::string_view fen);
    
    void synchronize() override;

    std::vector<int> get_HKP(bool color);

    modified_features get_modified_features(chess::Move move, bool color);

    void update_state(chess::Move move) override;

    void restore_state(chess::Move move) override;

    bool is_basic_move(chess::Move move) override;
    
    float evaluate() override;

};
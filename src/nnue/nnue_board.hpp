#pragma once 

#include <stack>

#include "chess.hpp"
#include "tbprobe.hpp"
#include "piece_square_tables.hpp"
#include "nnue.hpp"
#include "misc.hpp"

class NnueBoard: public Board {
    public:

    NnueBoard();
    NnueBoard(std::string_view fen);
    
    ~NnueBoard();

    void synchronize();

    bool legal(Move move);

    void update_state(Move move);

    void restore_state(Move move);

    int evaluate();

    bool is_stalemate();

    std::pair<std::vector<int>, std::vector<int>> get_features();

    private:
    AccumulatorsStack accumulators_stack;

    modified_features get_modified_features(Move move, Color color);

    bool is_updatable_move(Move move);
};
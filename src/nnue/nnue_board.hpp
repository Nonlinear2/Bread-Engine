#pragma once 

#include <stack>
#include <random>

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

    bool last_move_null();

    void update_state(Move move);

    void restore_state(Move move);

    int evaluate();

    bool try_outcome_eval(int& eval);


    bool probe_wdl(int& eval);


    bool probe_root_dtz(Move& move, Movelist& moves, bool generate_moves);

    Move tb_result_to_move(unsigned int tb_result);
    
    private:
    std::random_device rdev;
    std::mt19937 rgen;
    std::uniform_int_distribution<int> idist;

    std::stack<Accumulator> accumulator_stack;

    std::vector<int> get_HKP(bool color);

    modified_features get_modified_features(Move move, bool color);

    bool is_updatable_move(Move move);
};
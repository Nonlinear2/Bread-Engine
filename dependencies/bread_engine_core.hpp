#pragma once

#include <vector>
#include <chrono>
#include <atomic>
#include "chess.hpp"
#include "transposition_table.hpp"
#include "misc.hpp"
#include "nnue_board.hpp"
#include "sorted_move_gen.hpp"

class Engine {
    public:
    int nodes = 0;
    int current_depth = 0;
    std::atomic<SearchLimit> limit;
    std::atomic<bool> interrupt_flag = false;

    std::atomic<int> run_time;
    TranspositionTable transposition_table;

    NnueBoard inner_board = NnueBoard();

    const int QSEARCH_MAX_DEPTH = 6;

    bool try_outcome_eval(int& eval);

    int increment_mate_ply(int eval);

    bool is_mate(int eval);

    bool is_win(int eval);

    int get_think_time(int time_left, int num_moves_out_of_book, int num_moves_until_time_control, int increment);

    void update_run_time();

    chess::Move search(std::string fen, SearchLimit limit);
    chess::Move search(SearchLimit limit);

    chess::Move iterative_deepening(SearchLimit limit);

    private:
    friend class UCIAgent;

    int engine_color;
    std::chrono::time_point<std::chrono::high_resolution_clock> start_time;

    bool update_interrupt_flag();
    std::pair<std::string, std::string> get_pv_pmove(std::string fen);
    chess::Move minimax_root(int depth, int color);

    template<bool pv>
    int negamax(int depth, int color, int alpha, int beta);

    template<bool pv>
    int qsearch(int alpha, int beta, int color, int depth);
};
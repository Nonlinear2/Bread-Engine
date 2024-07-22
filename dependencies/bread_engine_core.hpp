#pragma once

#include <vector>
#include <chrono>
#include <atomic>
#include "chess.hpp"
#include "transposition_table.hpp"
#include "misc.hpp"
#include "nnue_board.hpp"
#include "sorted_move_gen.hpp"

// shorter name
inline std::chrono::time_point<std::chrono::high_resolution_clock> now(){
    return std::chrono::high_resolution_clock::now();
}

class Engine {
    public:
    int nodes = 0;
    int current_depth = 0;
    std::atomic<SearchLimit> limit;
    std::atomic<bool> interrupt_flag = false;

    std::atomic<float> run_time;
    TranspositionTable transposition_table;

    NnueBoard inner_board = NnueBoard();

    const int QSEARCH_MAX_DEPTH = 6;

    bool try_outcome_eval(float& eval);

    float increment_mate_ply(float eval);

    bool is_mate(float eval);

    bool is_win(float eval);

    float get_think_time(float time_left, int num_moves_out_of_book, int num_moves_until_time_control, int increment);

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
    float negamax(int depth, int color, float alpha, float beta);

    template<bool pv>
    float qsearch(float alpha, float beta, int color, int depth);
};
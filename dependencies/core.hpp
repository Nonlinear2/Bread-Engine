#pragma once

#include <vector>
#include <chrono>
#include <atomic>
#include "chess.hpp"
#include "transposition_table.hpp"
#include "misc.hpp"
#include "nonsense.hpp"
#include "nnue_board.hpp"
#include "sorted_move_gen.hpp"

struct Stack {
    chess::Move current_move = NO_MOVE;
};

class Engine {
    public:

    int nodes = 0;
    int current_depth = 0;
    std::atomic<SearchLimit> limit;
    std::atomic<bool> interrupt_flag = false;

    Stack stack[ENGINE_MAX_DEPTH + QSEARCH_MAX_DEPTH + 1] = {};
    Stack* root_ss = stack + 1;

    std::atomic<int> run_time;
    TranspositionTable transposition_table;

    NnueBoard inner_board = NnueBoard();

    void set_uci_display(bool v);

    int increment_mate_ply(int eval);

    bool is_mate(int eval);

    bool is_win(int eval);
    bool is_loss(int eval);

    int get_mate_in_moves(int eval);
    
    int get_think_time(int time_left, int num_moves_out_of_book, int num_moves_until_time_control, int increment);

    void update_run_time();

    chess::Move search(std::string fen, SearchLimit limit);
    chess::Move search(SearchLimit limit);

    chess::Move iterative_deepening(SearchLimit limit);

    std::atomic<bool> is_nonsense = false;

    private:
    friend class UCIAgent;

    bool display_uci = true;

    int engine_color;
    std::chrono::time_point<std::chrono::high_resolution_clock> start_time;

    Nonsense nonsense = Nonsense();

    bool update_interrupt_flag();
    std::pair<std::string, std::string> get_pv_pmove(std::string fen);
    chess::Move minimax_root(int depth, int color, Stack* ss);

    template<bool pv>
    int negamax(int depth, int color, int alpha, int beta, Stack* ss);

    template<bool pv>
    int qsearch(int alpha, int beta, int color, int depth, Stack* ss);
};
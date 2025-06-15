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

    Stack stack[ENGINE_MAX_DEPTH + QSEARCH_MAX_DEPTH + 2] = {};
    Stack* root_ss = stack + 2;

    std::atomic<int> run_time;
    TranspositionTable transposition_table;

    NnueBoard inner_board = NnueBoard();

    chess::Movelist root_moves;
    
    void set_uci_display(bool v);

    int increment_mate_ply(int value);

    bool is_mate(int value);

    bool is_decisive(int value);
    bool is_win(int value);
    bool is_loss(int value);

    int get_mate_in_moves(int value);
    
    int get_think_time(int time_left, int num_moves_out_of_book, int num_moves_until_time_control, int increment);

    void update_run_time();

    chess::Move search(std::string fen, SearchLimit limit);
    chess::Move search(SearchLimit limit);

    chess::Move iterative_deepening(SearchLimit limit);

    std::atomic<bool> is_nonsense = false;

    private:
    friend class UCIAgent;

    bool display_uci = true;

    std::chrono::time_point<std::chrono::high_resolution_clock> start_time;

    Nonsense nonsense = Nonsense();

    bool update_interrupt_flag();
    std::pair<std::string, std::string> get_pv_pmove();
    chess::Move minimax_root(int depth, Stack* ss);

    template<bool pv>
    int negamax(int depth, int alpha, int beta, Stack* ss);

    template<bool pv>
    int qsearch(int alpha, int beta, int depth, Stack* ss);
};
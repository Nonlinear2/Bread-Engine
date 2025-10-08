#pragma once

#include <vector>
#include <chrono>
#include <atomic>
#include "chess.hpp"
#include "transposition_table.hpp"
#include "misc.hpp"
#include "constants.hpp"
#include "history.hpp"
#include "see.hpp"
#include "nonsense.hpp"
#include "nnue_board.hpp"
#include "sorted_move_gen.hpp"
#include "tune.hpp"

int nnue_evaluate(NnueBoard& pos);

class Engine {
    public:

    int64_t nodes = 0;
    int current_depth = 0;
    std::atomic<SearchLimit> limit;
    std::atomic<bool> interrupt_flag = false;
    
    Stack stack[MAX_PLY + STACK_PADDING_SIZE] = {};
    Stack* root_ss = stack + 2;

    std::atomic<int> run_time;
    TranspositionTable transposition_table;

    NnueBoard pos = NnueBoard();

    Movelist root_moves;
    
    int get_think_time(float time_left, int num_moves_out_of_book,
        int num_moves_until_time_control, int increment);

    void update_run_time();

    Move search(std::string fen, SearchLimit limit);
    Move search(SearchLimit limit);

    Move iterative_deepening(SearchLimit limit);

    void clear_state();

    void save_state(std::string file);
    void load_state(std::string file);

    std::atomic<bool> is_nonsense = false;
    bool nonsense_active = false;

    private:
    friend class UCIAgent;

    std::chrono::time_point<std::chrono::high_resolution_clock> start_time;

    bool root_tb_hit;
    Move tb_move;
    Movelist tb_moves;

    int (*evaluate)(NnueBoard& pos) = nnue_evaluate;

    bool update_interrupt_flag();
    std::pair<std::string, std::string> get_pv_pmove();

    template<bool pv>
    int negamax(int depth, int alpha, int beta, Stack* ss);

    template<bool pv>
    int qsearch(int alpha, int beta, int depth, Stack* ss);
};
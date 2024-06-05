#pragma once

#include <vector>
#include <chrono>
#include "chess.hpp"
#include "transposition_table.hpp"

#include "nnue_board.hpp"
#define SEARCH_BOARD NnueBoard

#define NO_MOVE chess::Move::NO_MOVE

// shorter name
inline std::chrono::time_point<std::chrono::high_resolution_clock> now(){
    return std::chrono::high_resolution_clock::now();
}

const std::vector<int> piece_value = {
    1, // pawn
    3, // knight
    3, // bishop
    5, // rook
    9, // queen
    2, // king
    0, // none
};

class CircularBuffer3 {
    public:
    int curr_idx = 0;
    std::vector<uint16_t> data = std::vector<uint16_t>(3);

    void add_move(chess::Move move);

    bool in_buffer(chess::Move move);
};

class TerminateSearch: public std::exception {};


class Engine {
    public:
    int nodes = 0;
    int search_depth = 0; // current search depth
    int min_depth = 0;
    int max_depth = 0;
    std::atomic<int> time_limit; // milliseconds
    std::atomic<float> run_time;
    TranspositionTable transposition_table;

    SEARCH_BOARD inner_board = SEARCH_BOARD();

    int KILLER_SCORE = 10;
    int MATERIAL_CHANGE_MULTIPLIER = 6;
    int ENDGAME_PIECE_COUNT = 11;

    const int QSEARCH_MAX_DEPTH = 6;

    void set_move_score(chess::Move& move, int depth);

    void set_capture_score(chess::Move& move);

    void order_moves(chess::Movelist& moves, uint64_t zobrist_hash, int depth);

    void order_captures(chess::Movelist& moves);

    bool try_outcome_eval(float& eval);

    float get_think_time(float time_left, int num_moves_out_of_book, int num_moves_until_time_control, int increment);

    void update_run_time();

    chess::Move search(int time_limit_, int min_depth_, int max_depth_);
    chess::Move search(std::string fen, int time_limit_, int min_depth_, int max_depth_);

    void set_interrupt_flag();
    void unset_interrupt_flag();

    chess::Move iterative_deepening(int time_limit_, int min_depth_, int max_depth_);

    private:
    friend class UCIAgent;
    
    int engine_color;
    size_t engine_max_depth = 20;
    std::atomic<bool> interrupt_flag = false;
    PieceSquareMaps psm = PieceSquareMaps();
    std::vector<CircularBuffer3> killer_moves{engine_max_depth};
    std::chrono::time_point<std::chrono::high_resolution_clock> start_time;

    float get_outcome_eval(int depth);
    bool can_return();
    std::pair<std::string, std::string> get_pv_pmove(std::string fen);
    std::pair<chess::Move, TFlag> minimax_root(int depth, int color, float alpha, float beta);
    // chess::Move aspiration_search(int depth, int color, float prev_eval);
    float negamax(int depth, int color, float alpha, float beta);
    float qsearch(float alpha, float beta, int color, int depth);
};
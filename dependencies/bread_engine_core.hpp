#pragma once

#include <vector>
#include <chrono>
#include <atomic>
#include "chess.hpp"
#include "transposition_table.hpp"

#include "nnue_board.hpp"
#define SEARCH_BOARD NnueBoard

#define NO_MOVE chess::Move::NO_MOVE
#define ENGINE_MAX_DEPTH 25

#define BEST_MOVE_SCORE 1000
#define WORST_MOVE_SCORE -1000

#define WORST_EVAL -100
#define BEST_EVAL 100

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
    std::array<uint16_t, 3> data;

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

    const int QSEARCH_MAX_DEPTH = 6;

    bool try_outcome_eval(float& eval);

    float get_think_time(float time_left, int num_moves_out_of_book, int num_moves_until_time_control, int increment);

    void update_run_time();

    chess::Move search(int time_limit, int min_depth, int max_depth);
    chess::Move search(std::string fen, int time_limit, int min_depth, int max_depth);

    void set_interrupt_flag();
    void unset_interrupt_flag();

    chess::Move iterative_deepening(int time_limit, int min_depth, int max_depth);

    template<chess::movegen::MoveGenType MoveGenType>
    class SortedMoveGen {
        public:
        static constexpr PieceSquareMaps psm = PieceSquareMaps();
        static inline float KILLER_SCORE = 14.9;
        static inline float MATERIAL_CHANGE_MULTIPLIER = 11.9;
        static inline float ENDGAME_PIECE_COUNT = 11;

        static std::array<CircularBuffer3, ENGINE_MAX_DEPTH> killer_moves;

        NnueBoard& board;

        SortedMoveGen(NnueBoard& board);
        SortedMoveGen(NnueBoard& board, int depth);
        void generate_moves();
        void set_score(chess::Move& move, int depth); // for all moves
        void set_score(chess::Move& move); // for capture moves
        void set_tt_move(chess::Move move);
        bool next(chess::Move& move);
        bool is_empty();
        int index();
        static void clear_killer_moves();
        chess::Movelist legal_moves;
        private:
        int depth;
        int move_idx = -1;
        bool checked_tt_move = false;
        chess::Move tt_move = NO_MOVE;
    };
    private:
    friend class UCIAgent;

    int engine_color;
    std::atomic<bool> interrupt_flag = false;
    std::chrono::time_point<std::chrono::high_resolution_clock> start_time;

    bool can_return();
    std::pair<std::string, std::string> get_pv_pmove(std::string fen);
    std::pair<chess::Move, TFlag> minimax_root(int depth, int color, float alpha, float beta);
    float negamax(int depth, int color, float alpha, float beta);
    float qsearch(float alpha, float beta, int color, int depth);
};
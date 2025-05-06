#pragma once

#include <array>

#include "piece_square_tables.hpp"
#include "nnue_board.hpp"
#include "misc.hpp"
#include "chess.hpp"

template<chess::movegen::MoveGenType MoveGenType>
class SortedMoveGen: public chess::Movelist {
    public:
    static constexpr PieceSquareMaps psm = PieceSquareMaps();
    static inline int KILLER_SCORE = 149;
    static inline int MATERIAL_CHANGE_MULTIPLIER = 119;
    static inline int ENDGAME_PIECE_COUNT = 11;

    static inline std::array<CircularBuffer3, ENGINE_MAX_DEPTH> killer_moves = {};
    static inline History history = History();

    NnueBoard& board;

    SortedMoveGen(NnueBoard& board);
    SortedMoveGen(NnueBoard& board, int depth);
    void generate_moves();
    void set_tt_move(chess::Move move);
    bool next(chess::Move& move);
    bool is_empty();
    int index();
    static void clear_killer_moves();
    void update_history(chess::Move move, int depth, bool color);

    chess::Move tt_move = NO_MOVE;
    int generated_moves_count = 0;
    private:
    int depth = DEPTH_UNSEARCHED;
    int move_idx = -1;
    bool checked_tt_move = false;
    bool generated_moves = false;
    void set_score(chess::Move& move);
    bool is_valid_move(chess::Move move);
    chess::Move pop_move(int move_idx);
    chess::Move pop_best_score();
};
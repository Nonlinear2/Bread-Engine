#pragma once

#include <array>

#include "piece_square_tables.hpp"
#include "nnue_board.hpp"
#include "misc.hpp"
#include "chess.hpp"

const std::vector<int> piece_value = {
    1, // pawn
    3, // knight
    3, // bishop
    5, // rook
    9, // queen
    2, // king
    0, // none
};

template<chess::movegen::MoveGenType MoveGenType>
class SortedMoveGen: public chess::Movelist {
    public:
    static constexpr PieceSquareMaps psm = PieceSquareMaps();
    static inline float KILLER_SCORE = 14.9;
    static inline float MATERIAL_CHANGE_MULTIPLIER = 11.9;
    static inline float ENDGAME_PIECE_COUNT = 11;

    static inline std::array<CircularBuffer3, ENGINE_MAX_DEPTH> killer_moves = {};

    NnueBoard& board;

    SortedMoveGen(NnueBoard& board);
    SortedMoveGen(NnueBoard& board, int depth);
    void generate_moves();
    void set_tt_move(chess::Move move);
    bool next(chess::Move& move);
    bool is_empty();
    int index();
    static void clear_killer_moves();

    private:
    int depth;
    int move_idx = -1;
    int num_remaining_moves;
    bool checked_tt_move = false;
    bool assigned_scores = false;
    chess::Move tt_move = NO_MOVE;
    void set_score(chess::Move& move);
    bool is_valid_move(chess::Move move);
    chess::Move pop_move(int move_idx);
    chess::Move pop_best_score();
};
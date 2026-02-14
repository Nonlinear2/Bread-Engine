#pragma once

#include <array>

#include "chess.hpp"
#include "nnue_board.hpp"
#include "constants.hpp"
#include "history.hpp"
#include "misc.hpp"
#include "see.hpp"
#include "tune.hpp"

enum GenerationStage: int {
    TT_MOVE,
    GENERATE_CAPTURES,
    GOOD_CAPTURES,
    GENERATE_QUIETS,
    QUIETS,
    BAD_CAPTURES,
};


enum GenType: int {
    NORMAL,
    QSEARCH,
};

constexpr GenerationStage& operator++(GenerationStage& g) {
    return g = static_cast<GenerationStage>(static_cast<int>(g) + 1);
}

template<GenType MoveGenType>
class SortedMoveGen {
    public:

    static inline KillerMoves killer_moves = KillerMoves();
    static inline FromToHistory history = FromToHistory();
    static inline ContinuationHistory cont_history = ContinuationHistory();

    Piece prev_piece;
    Square prev_to;
    NnueBoard& pos;

    SortedMoveGen(Movelist* to_search, Piece prev_piece, Square prev_to, NnueBoard& pos, int depth);
    SortedMoveGen(Piece prev_piece, Square prev_to, NnueBoard& pos);
    void set_tt_move(Move move);
    bool next(Move& move);
    bool empty();
    int index();
    void update_history(Move best_move, int depth);
    void update_cont_history(Piece prev_piece, Square prev_to, Piece piece, Square to, int bonus);
    void set_score(Move& move);
    void prepare_pos_data();

    Move tt_move = Move::NO_MOVE;
    private:
    Movelist moves;
    Movelist bad_captures;

    Bitboard attacked_by_pawn;
    std::array<Bitboard, 6> check_squares;
    Movelist* to_search = NULL;

    int depth = DEPTH_UNSEARCHED;
    int move_idx = -1;

    GenerationStage stage = TT_MOVE;
    Move pop_move(Movelist& ml, int move_idx);
    Move pop_best_score(Movelist& ml);
};
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

    Piece prev_piece;
    Square prev_to;
    Piece prev_piece2 = Piece::NONE;
    Square prev_to2 = Square::NO_SQ;
    NnueBoard& pos;

    Move tt_move = Move::NO_MOVE;
    std::array<Bitboard, 6> check_squares;

    SortedMoveGen(Movelist* to_search, Piece prev_piece, Square prev_to,
        Piece prev_piece2, Square prev_to2, NnueBoard& pos, int depth,
        KillerMoves& killers, FromToHistory& hist,
        ContinuationHistory& cont_hist, ContinuationHistory& cont_hist2, CaptureHistory& capt_hist);
    SortedMoveGen(Piece prev_piece, Square prev_to, NnueBoard& pos,
        KillerMoves& killers, FromToHistory& hist, ContinuationHistory& cont_hist, CaptureHistory& capt_hist);

    void set_tt_move(Move move);
    bool next(Move& move);
    bool empty();
    int index();
    void update_history(Move best_move, int depth);
    void update_capture_history(Move best_move, int depth);
    void update_cont_history(Piece prev_piece, Square prev_to, Piece piece, Square to, int bonus);
    void update_cont_history2(Piece prev_piece2, Square prev_to2, Piece piece, Square to, int bonus);
    void set_score(Move& move);
    void prepare_capture_sort();
    void prepare_quiet_sort();
    void skip_quiets();

    private:

    KillerMoves& killer_moves;
    FromToHistory& history;
    ContinuationHistory& cont_history;
    ContinuationHistory& cont_history2;
    CaptureHistory& capt_history;

    bool skip_quiets_ = false;
    Movelist moves;
    Movelist bad_captures;

    Bitboard attacked_by_pawn;
    Movelist* to_search = NULL;

    int depth = DEPTH_UNSEARCHED;
    int move_idx = 0;
    int bad_capture_idx = 0;

    GenerationStage stage = TT_MOVE;

    Move pop_move(Movelist& ml, int idx);
    Move pop_best_score(Movelist& ml);
};
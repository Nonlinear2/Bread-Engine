#pragma once

#include <array>

#include "piece_square_tables.hpp"
#include "nnue_board.hpp"
#include "misc.hpp"
#include "chess.hpp"

enum GenerationStage: int{
    TT_MOVE,
    GENERATE_MOVES,
    GET_MOVES,
};

constexpr GenerationStage& operator++(GenerationStage& g) {
    return g = static_cast<GenerationStage>(static_cast<int>(g) + 1);
}

template<movegen::MoveGenType MoveGenType>
class SortedMoveGen {
    public:
    static constexpr PieceSquareMaps psm = PieceSquareMaps();

    static inline std::array<CircularBuffer3, ENGINE_MAX_DEPTH> killer_moves = {};
    static inline History history = History();

    NnueBoard& pos;

    SortedMoveGen(NnueBoard& pos);
    SortedMoveGen(NnueBoard& pos, int depth);
    void set_tt_move(Move move);
    bool next(Move& move);
    bool empty();
    int index();
    static void clear_killer_moves();
    void update_history(Move move, int depth, bool color);
    void set_score(Move& move);
    void prepare_pos_data();

    Move tt_move = Move::NO_MOVE;
    private:
    Movelist moves;
    
    Bitboard attacked_by_pawn;
    std::vector<Bitboard> check_squares;
    bool is_endgame;

    int depth = DEPTH_UNSEARCHED;
    int move_idx = -1;
    GenerationStage stage = TT_MOVE;
    Move pop_move(int move_idx);
    Move pop_best_score();
};
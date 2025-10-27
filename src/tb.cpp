#include "tb.hpp"

bool TB::probe_wdl(Board& pos, int& eval){
    if (pos.occ().count() > TB_LARGEST)
        return false;

    unsigned int ep_square = pos.enpassantSq().index();
    if (ep_square == 64)
        ep_square = 0;

    unsigned int TB_hit = tb_probe_wdl(
            pos.us(Color::WHITE).getBits(), pos.us(Color::BLACK).getBits(), 
            pos.pieces(PieceType::KING).getBits(), pos.pieces(PieceType::QUEEN).getBits(),
            pos.pieces(PieceType::ROOK).getBits(), pos.pieces(PieceType::BISHOP).getBits(),
            pos.pieces(PieceType::KNIGHT).getBits(), pos.pieces(PieceType::PAWN).getBits(),
            pos.halfMoveClock(), pos.castlingRights().has(pos.sideToMove()),
            ep_square, pos.sideToMove() == Color::WHITE
    );
    switch(TB_hit){
        case TB_WIN:
            eval = TB_VALUE;
            return true;
        case TB_LOSS:
            eval = -TB_VALUE;
            return true;
        case TB_DRAW:
        case TB_CURSED_WIN:
        case TB_BLESSED_LOSS:
            eval = 0;
            return true;
        default:
            return false;
    }
}

bool TB::probe_root_dtz(Board& pos, Move& move, Movelist& moves, bool generate_moves){
    if (pos.occ().count() > TB_LARGEST)
        return false;

    unsigned int ep_square = pos.enpassantSq().index();
    if (ep_square == 64)
        ep_square = 0;

    unsigned int tb_moves[TB_MAX_MOVES];

    unsigned int TB_hit = tb_probe_root(
            pos.us(Color::WHITE).getBits(), pos.us(Color::BLACK).getBits(), 
            pos.pieces(PieceType::KING).getBits(), pos.pieces(PieceType::QUEEN).getBits(),
            pos.pieces(PieceType::ROOK).getBits(), pos.pieces(PieceType::BISHOP).getBits(),
            pos.pieces(PieceType::KNIGHT).getBits(), pos.pieces(PieceType::PAWN).getBits(),
            pos.halfMoveClock(), pos.castlingRights().has(pos.sideToMove()),
            ep_square, pos.sideToMove() == Color::WHITE,
            generate_moves ? tb_moves : NULL
    );

    if ((TB_hit == TB_RESULT_CHECKMATE) || (TB_hit == TB_RESULT_STALEMATE) || (TB_hit == TB_RESULT_FAILED)){
        return false;
    }

    move = tb_result_to_move(TB_hit);

    if (generate_moves){
        Move current_move;
        for (int i = 0; i < TB_MAX_MOVES; i++){
            if (tb_moves[i] == TB_RESULT_FAILED)
                break;
            current_move = tb_result_to_move(tb_moves[i]);
            if (current_move.score() == TB_VALUE){
                current_move.setScore(NO_VALUE);
                moves.add(current_move);
            }
        }
    }
    return true;
}

Move TB::tb_result_to_move(unsigned int tb_result){
    Move move;
    if (TB_GET_PROMOTES(tb_result) == TB_PROMOTES_NONE){
        move = Move::make(
            static_cast<Square>(TB_GET_FROM(tb_result)),
            static_cast<Square>(TB_GET_TO(tb_result)));
    } else {
        PieceType promotion_type;

        switch (TB_GET_PROMOTES(tb_result)){
        case TB_PROMOTES_QUEEN:
            promotion_type = PieceType::QUEEN;
            break;
        case TB_PROMOTES_KNIGHT:
            promotion_type = PieceType::KNIGHT;
            break;
        case TB_PROMOTES_ROOK:
            promotion_type = PieceType::ROOK;
            break;
        case TB_PROMOTES_BISHOP:
            promotion_type = PieceType::BISHOP;
            break;
        }

        move = Move::make<Move::PROMOTION>(
            static_cast<Square>(TB_GET_FROM(tb_result)),
            static_cast<Square>(TB_GET_TO(tb_result)),
            promotion_type);
    }

    switch(TB_GET_WDL(tb_result)){
        case TB_WIN:
            move.setScore(TB_VALUE);
            break;
        case TB_LOSS:
            move.setScore(-TB_VALUE);
            break;
        default: // TB_DRAW, TB_CURSED_WIN, TB_BLESSED_LOSS
            move.setScore(0);
    }

    return move;
}
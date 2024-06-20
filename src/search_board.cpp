#include "search_board.hpp"

bool SearchBoard::probe_wdl(float& eval){
    if (occ().count() > 5){
        return false;
    }
    unsigned int TB_hit = tb_probe_wdl(
            us(chess::Color::WHITE).getBits(), us(chess::Color::BLACK).getBits(), 
            pieces(chess::PieceType::KING).getBits(), pieces(chess::PieceType::QUEEN).getBits(),
            pieces(chess::PieceType::ROOK).getBits(), pieces(chess::PieceType::BISHOP).getBits(),
            pieces(chess::PieceType::KNIGHT).getBits(), pieces(chess::PieceType::PAWN).getBits(),
            halfMoveClock(), castlingRights().has(sideToMove()),
            enpassantSq().index(), sideToMove() == chess::Color::WHITE
    );
    switch(TB_hit){
        case TB_WIN:
            eval = 1;
            return true;
        case TB_LOSS:
            eval = -1;
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

bool SearchBoard::probe_dtz(chess::Move& move){
    if (occ().count() > TB_LARGEST){
        return false;
    }

    unsigned int ep_square = enpassantSq().index();
    if (ep_square == 64) ep_square = 0;

    unsigned int TB_hit = tb_probe_root(
            us(chess::Color::WHITE).getBits(), us(chess::Color::BLACK).getBits(), 
            pieces(chess::PieceType::KING).getBits(), pieces(chess::PieceType::QUEEN).getBits(),
            pieces(chess::PieceType::ROOK).getBits(), pieces(chess::PieceType::BISHOP).getBits(),
            pieces(chess::PieceType::KNIGHT).getBits(), pieces(chess::PieceType::PAWN).getBits(),
            halfMoveClock(), castlingRights().has(sideToMove()),
            ep_square, sideToMove() == chess::Color::WHITE,
            NULL
    );

    if ((TB_hit == TB_RESULT_CHECKMATE) || (TB_hit == TB_RESULT_STALEMATE) || (TB_hit == TB_RESULT_FAILED)){
        return false;
    }

    if (TB_GET_PROMOTES(TB_hit) == TB_PROMOTES_NONE){
        move = chess::Move::make(
            static_cast<chess::Square>(TB_GET_FROM(TB_hit)),
            static_cast<chess::Square>(TB_GET_TO(TB_hit)));
    } else {
        chess::PieceType promotion_type;

        switch (TB_GET_PROMOTES(TB_hit)){
        case TB_PROMOTES_QUEEN:
            promotion_type = chess::PieceType::QUEEN;
            break;
        case TB_PROMOTES_KNIGHT:
            promotion_type = chess::PieceType::KNIGHT;
            break;
        case TB_PROMOTES_ROOK:
            promotion_type = chess::PieceType::ROOK;
            break;
        case TB_PROMOTES_BISHOP:
            promotion_type = chess::PieceType::BISHOP;
            break;
        }

        move = chess::Move::make<chess::Move::PROMOTION>(
            static_cast<chess::Square>(TB_GET_FROM(TB_hit)),
            static_cast<chess::Square>(TB_GET_TO(TB_hit)),
            promotion_type);
    }
    switch(TB_GET_WDL(TB_hit)){
        case TB_WIN:
            move.setScore(1000);
            break;
        case TB_LOSS:
            move.setScore(-1000);
            break;
        default: // TB_DRAW, TB_CURSED_WIN, TB_BLESSED_LOSS
            move.setScore(0);
            break;
    }
    return true;
}

#include "nonsense.hpp"

void Nonsense::display_info(){
    if (rand() % rick_astley_odds == 0){
        int r = rand() % (rick_astley_lyrics.size() + num_main_lyrics); 
        r %= rick_astley_lyrics.size(); // this makes the main lyrics twice as common
        std::cout << "info string " << rick_astley_lyrics[r] << std::endl;
    }
}

Move Nonsense::play_bongcloud(const Board& pos){
    Movelist legal_moves;

    movegen::legalmoves(legal_moves, pos);

    Move pawn_move;
    Move king_move;
    if (pos.sideToMove() == Color::WHITE){
        pawn_move = Move::make(Square::SQ_E2, Square::SQ_E4, PieceType::PAWN);
        king_move = Move::make(Square::SQ_E1, Square::SQ_E2, PieceType::KING);
    } else {
        pawn_move = Move::make(Square::SQ_E7, Square::SQ_E5, PieceType::PAWN);
        king_move = Move::make(Square::SQ_E8, Square::SQ_E7, PieceType::KING);
    }

    bool pawn_move_legal = std::find(legal_moves.begin(), legal_moves.end(), pawn_move) != legal_moves.end();
    bool king_move_legal = std::find(legal_moves.begin(), legal_moves.end(), king_move) != legal_moves.end();
    if (pos.fullMoveNumber() == 1 && pawn_move_legal){
        std::cout << "info depth 1";
        std::cout << " score cp 0";
        std::cout << " nodes 0 nps 0";
        std::cout << " time 0";
        std::cout << " hashfull 0";
        std::cout << " pv " << uci::moveToUci(pawn_move) << std::endl;
        std::cout << "bestmove " << uci::moveToUci(pawn_move) << std::endl;
        pawn_move.setScore(0);
        return pawn_move;
    } else if (pos.fullMoveNumber() == 2 && king_move_legal){
        std::cout << "info depth 91";
        std::cout << " score mate 78";
        std::cout << " nodes 0 nps 0";
        std::cout << " time 0";
        std::cout << " hashfull 0";
        std::cout << " pv " << uci::moveToUci(king_move) << std::endl;
        std::cout << "bestmove " << uci::moveToUci(king_move) << std::endl;
        king_move.setScore(0);
        return king_move;
    }
    return Move::NO_MOVE;
}

bool Nonsense::is_winning_side(Board& pos){
    return pos.us(pos.sideToMove()).count() > 1;
}

bool Nonsense::enough_material_for_nonsense(Board& pos){
    Color stm = pos.sideToMove();

    // enough pawns
    if (pos.pieces(PieceType::PAWN, stm).count() >= 2)
        return true;

    // bishops
    if (pos.pieces(PieceType::BISHOP, stm).count() >= 2
        && (pos.pieces(PieceType::BISHOP, stm) & 0x55aa55aa55aa55aa) // has a light square bishop 
        && (pos.pieces(PieceType::BISHOP, stm) & 0xaa55aa55aa55aa55)) // has a dark square bishop 
        return true;

    // knights
    if (pos.pieces(PieceType::KNIGHT, stm).count() > 2)
        return true;

    // bishops and knights
    if (pos.pieces(PieceType::KNIGHT, stm) && pos.pieces(PieceType::BISHOP, stm))
        return true;
    
    return false;
}

int Nonsense::material_evaluate(Board& pos){
    Color stm = pos.sideToMove();
    int eval = 0;
    for (PieceType pt: {PieceType::PAWN, PieceType::KNIGHT, PieceType::BISHOP, PieceType::ROOK, PieceType::QUEEN})
        eval += (pos.pieces(pt, stm).count() - pos.pieces(pt, !stm).count())
            * piece_value[static_cast<int>(pt)];
    return eval;
}

int Nonsense::evaluate(NnueBoard& pos){
    Color stm = pos.sideToMove();
    assert(pos.us(stm).count() == 1 || pos.them(stm).count() == 1); // make sure we are in a vs king endgame
    
    int eval = (is_winning_side(pos) ? -1 : 1) * 
        Square::distance(pos.kingSq(stm), pos.kingSq(!stm));

    eval += (is_winning_side(pos) ? 1 : -1) * only_knight_bishop(pos) * 1000;

    Bitboard us_pawns = pos.pieces(PieceType::PAWN, stm);
    while (us_pawns){
        Square sq = us_pawns.pop();
        eval += psm.get_psm(pos.at(sq), sq);
    }

    Bitboard them_pawns = pos.pieces(PieceType::PAWN, !stm);
    while (them_pawns){
        Square sq = them_pawns.pop();
        eval -= psm.get_psm(pos.at(sq), sq);
    }

    for (PieceType pt: {PieceType::PAWN, PieceType::KNIGHT, PieceType::BISHOP})
        eval += (pos.pieces(pt, stm).count() - pos.pieces(pt, !stm).count())
            * nonsense_piece_value[static_cast<int>(pt)];

    return std::clamp(eval, -BEST_VALUE, BEST_VALUE);
}

bool Nonsense::only_knight_bishop(NnueBoard& pos){
    // if (pos.halfMoveClock() >= 70)
    //     return false; // avoid not checkmating with queen or rook when there is no other choice

    return !bool(pos.pieces(PieceType::PAWN) | pos.pieces(PieceType::QUEEN) | pos.pieces(PieceType::ROOK));    
}
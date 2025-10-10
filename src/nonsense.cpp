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

    if (pos.fullMoveNumber() == 1 && legal_moves.find(pawn_move)){
        std::cout << "info depth 1";
        std::cout << " score cp 0";
        std::cout << " nodes 0 nps 0";
        std::cout << " time 0";
        std::cout << " hashfull 0";
        std::cout << " pv " << uci::moveToUci(pawn_move) << std::endl;
        std::cout << "bestmove " << uci::moveToUci(pawn_move) << std::endl;

        pawn_move.setScore(0);
        return pawn_move;
    } else if (pos.fullMoveNumber() == 2 && legal_moves.find(king_move)){
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
}

bool Nonsense::should_use_nonsense_eval(Board& pos){
    Color stm = pos.sideToMove();
    if (pos.them(stm).count() > 1)
        return false;

    // queen or rook
    if (pos.pieces(PieceType::QUEEN, stm) || pos.pieces(PieceType::ROOK, stm))
        return true;

    // enough pawns
    if (pos.pieces(PieceType::PAWN).count() >= 2)
        return true;

    // bishops
    if (pos.pieces(PieceType::BISHOP).count() >= 2
        && (pos.pieces(PieceType::BISHOP) & 0x55aa55aa55aa55aa) // has a light square bishop 
        && (pos.pieces(PieceType::BISHOP) & 0xaa55aa55aa55aa55)) // has a dark square bishop 
        return true;

    // knights
    if (pos.pieces(PieceType::KNIGHT).count() > 2)
        return true;

    // bishops and knights
    if (pos.pieces(PieceType::KNIGHT) && pos.pieces(PieceType::BISHOP))
        return true;
    
    return false;
}

int Nonsense::evaluate(NnueBoard& pos){
    Color stm = pos.sideToMove();

    assert(pos.us(stm).count() == 1 || pos.them(stm).count() == 1); // make sure we are in a vs king endgame
    bool winning_side = pos.us(stm).count() > 1;

    int standard_eval = pos.evaluate();
    if (pos.pieces(PieceType::QUEEN) | pos.pieces(PieceType::ROOK)){
        if (winning_side)
            standard_eval = std::min(0, standard_eval);
        else
            standard_eval = std::max(0, standard_eval);
    }

    int material_eval = (winning_side ? -1 : 1)
        * Square::distance(pos.kingSq(stm), pos.kingSq(!stm))*(5 + 8*!pos.pieces(PieceType::PAWN));

    Bitboard pawns = pos.pieces(PieceType::PAWN);
    while (pawns){
        Square sq = pawns.pop();
        material_eval += (winning_side ? -1 : 1) * psm.get_psm(pos.at(sq), sq);
    }

    for (PieceType pt: {PieceType::KNIGHT, PieceType::BISHOP})
        material_eval += (pos.pieces(pt, stm).count() - pos.pieces(pt, !stm).count())
            * nonsense_piece_value[static_cast<int>(pt)];
    
    return std::clamp((standard_eval / 5 + material_eval), -BEST_VALUE, BEST_VALUE);
}

bool Nonsense::is_bad_position(NnueBoard& pos){
    // if (pos.halfMoveClock() >= 70)
    //     return false; // avoid not checkmating with queen or rook when there is no other choice

    return (bool)pos.pieces(PieceType::PAWN)
        || (pos.pieces(PieceType::QUEEN) | pos.pieces(PieceType::ROOK));
}

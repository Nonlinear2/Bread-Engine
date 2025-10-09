#include "nonsense.hpp"

void Nonsense::display_info(){
    if (rand() % rick_astley_odds == 0){
        int r = rand() % (rick_astley_lyrics.size() + num_main_lyrics); 
        r %= rick_astley_lyrics.size(); // this makes the main lyrics twice as common
        std::cout << "info string " << rick_astley_lyrics[r] << std::endl;
    }
}

bool Nonsense::should_bongcloud(uint64_t hash, int move_number){
    if (is_bongcloud)
        is_bongcloud = (move_number == 2); // make sure it is still possible to bongcloud.

    return (is_bongcloud || hash == starting_pos_hash);
}

Move Nonsense::play_bongcloud(){
    if (is_bongcloud){
        std::cout << "info depth 91";
        std::cout << " score mate 78";
        std::cout << " nodes 0 nps 0";
        std::cout << " time 0";
        std::cout << " hashfull 0";
        std::cout << " pv e1e2" << std::endl;
        std::cout << "bestmove e1e2" << std::endl;
        
        is_bongcloud = false;
        Move move = Move::make(Square(Square::underlying::SQ_E1),
                                             Square(Square::underlying::SQ_E2),
                                             PieceType::KING);
        move.setScore(10);
        return move;
    } else {
        std::cout << "info depth 1";
        std::cout << " score cp 0";
        std::cout << " nodes 0 nps 0";
        std::cout << " time 0";
        std::cout << " hashfull 0";
        std::cout << " pv e2e4" << std::endl;
        std::cout << "bestmove e2e4" << std::endl;

        is_bongcloud = true;
        Move move = Move::make(Square(Square::underlying::SQ_E2),
                                             Square(Square::underlying::SQ_E4),
                                             PieceType::PAWN);
        move.setScore(0);
        return move;
    }
}

bool Nonsense::is_theoretical_win(Board& pos){
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

int Nonsense::endgame_nonsense_evaluate(NnueBoard& pos){
    Color stm = pos.sideToMove();
    assert(pos.us(stm).count() == 1 || pos.them(stm).count() == 1); // make sure we are in a vs king endgame

    int standard_eval = pos.evaluate();
    int material_eval = 0;
    for (PieceType pt: {PieceType::PAWN, PieceType::KNIGHT, PieceType::BISHOP, PieceType::ROOK, PieceType::QUEEN})
        material_eval += (pos.pieces(pt, stm).count() - pos.pieces(pt, !stm).count())
            * nonsense_piece_value[static_cast<int>(pt)];
    material_eval += (material_eval > 0 ? -1 : 1)
        * Square::distance(pos.kingSq(stm), pos.kingSq(!stm))*(5 + 8*!pos.pieces(PieceType::PAWN));

    return std::clamp((standard_eval / 6 + material_eval), -BEST_VALUE, BEST_VALUE);
}

bool Nonsense::is_bad_checkmate(NnueBoard& pos){
    if (pos.halfMoveClock() >= 70)
        return false; // avoid not checkmating with queen or rook when there is no other choice

    return (bool)pos.pieces(PieceType::PAWN)
        || (pos.pieces(PieceType::QUEEN) | pos.pieces(PieceType::ROOK));
}

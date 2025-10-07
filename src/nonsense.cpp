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

Move Nonsense::play_bongcloud(bool display_info){
    if (is_bongcloud){
        if (display_info){
            std::cout << "info depth 91";
            std::cout << " score mate 78";
            std::cout << " nodes 149597870700 nps 299792458";
            std::cout << " time 0";
            std::cout << " hashfull 0";
            std::cout << " pv e1e2" << std::endl;
            std::cout << "bestmove e1e2" << std::endl;
        }
        is_bongcloud = false;
        Move move = Move::make(Square(Square::underlying::SQ_E1),
                                             Square(Square::underlying::SQ_E2),
                                             PieceType::KING);
        move.setScore(10);
        return move;
    } else {
        if (display_info){
            std::cout << "info depth 1";
            std::cout << " score cp 0";
            std::cout << " nodes 10 nps 3";
            std::cout << " time 0";
            std::cout << " hashfull 0";
            std::cout << " pv e2e4" << std::endl;
            std::cout << "bestmove e2e4" << std::endl;
        }
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
    if (pos.pieces(PieceType::BISHOP).count() > 2
        || !Square::same_color(pos.pieces(PieceType::BISHOP).lsb(), pos.pieces(PieceType::BISHOP).msb()))
        return true;

    // knights
    if (pos.pieces(PieceType::KNIGHT).count() > 2)
        return true;

    // bishops and knights
    if (pos.pieces(PieceType::KNIGHT) && pos.pieces(PieceType::BISHOP))
        return true;
    
    return false;
}

Move Nonsense::worst_winning_move(Board pos, Move move, Movelist moves){
    Move worst_winning_move = move;
    for (auto move: moves){
        if (move.score() != TB_VALUE)
            continue;

        pos.makeMove(move);
        Movelist next_moves;
        movegen::legalmoves(next_moves, pos);
        bool loses_material = false;
        for (Move m: next_moves)
            if (pos.isCapture(m)){
                loses_material = true;
                break;
            }
        pos.unmakeMove(move);
        
        if (loses_material)
            continue;

        if (move.typeOf() == Move::ENPASSANT){
            worst_winning_move = move;
            break;
        } else if (move.typeOf() == Move::PROMOTION){
            PieceType promotion = move.promotionType();
            if (promotion == PieceType::ROOK)
                worst_winning_move = move;

            if (promotion == PieceType::KNIGHT || promotion == PieceType::BISHOP){
                worst_winning_move = move;
                break;
            }
        } else if (pos.at(move.from()).type() == PieceType::PAWN)
            worst_winning_move = move;
    }
    return worst_winning_move;
}


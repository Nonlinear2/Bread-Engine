#include "nonsense.hpp"

void Nonsense::display_info(){
    if (rand() % rick_astley_odds == 0){
        int r = rand() % (rick_astley_lyrics.size() + num_main_lyrics); 
        r %= rick_astley_lyrics.size(); // this makes the main lyrics twice as common
        std::cout << "info string " << rick_astley_lyrics[r] << std::endl;
    }
}

bool Nonsense::should_bongcloud(uint64_t hash, int move_number){
    if (is_bongcloud) is_bongcloud = (move_number == 2); // make sure it is still possible to bongcloud.

    return (is_bongcloud || (hash == starting_pos_hash));
}

chess::Move Nonsense::play_bongcloud(bool display_info){
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
        chess::Move move = chess::Move::make(chess::Square(chess::Square::underlying::SQ_E1),
                                             chess::Square(chess::Square::underlying::SQ_E2),
                                             chess::PieceType::KING);
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
        chess::Move move = chess::Move::make(chess::Square(chess::Square::underlying::SQ_E2),
                                             chess::Square(chess::Square::underlying::SQ_E4),
                                             chess::PieceType::PAWN);
        move.setScore(0);
        return move;
    }
}

chess::Move Nonsense::worst_winning_move(chess::Move move, chess::Movelist moves){
    chess::Move worst_winning_move = move;
    for (auto move: moves){
        if (move.score() != TB_VALUE) continue;

        if (move.typeOf() == chess::Move::ENPASSANT){
            worst_winning_move = move;
            break;
        } else if (move.typeOf() == chess::Move::PROMOTION){
            chess::PieceType promotion = move.promotionType();
            if (promotion == chess::PieceType::ROOK) worst_winning_move = move;

            if ((promotion == chess::PieceType::KNIGHT) || (promotion == chess::PieceType::BISHOP)){
                worst_winning_move = move;
                break;
            }
        }
    }
    return worst_winning_move;
}


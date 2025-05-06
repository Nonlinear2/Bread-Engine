#include "sorted_move_gen.hpp"

template<>
SortedMoveGen<chess::movegen::MoveGenType::ALL>::SortedMoveGen(NnueBoard& board, int depth): board(board), depth(depth) {};

template<>
SortedMoveGen<chess::movegen::MoveGenType::CAPTURE>::SortedMoveGen(NnueBoard& board): board(board) {};

template<chess::movegen::MoveGenType MoveGenType>
void SortedMoveGen<MoveGenType>::generate_moves(){
    chess::movegen::legalmoves<MoveGenType>(*this, board);
    generated_moves_count = size_;
}

// set move score to be sorted later
template<>
void SortedMoveGen<chess::movegen::MoveGenType::ALL>::set_score(chess::Move& move){
    
    const chess::Square from = move.from();
    const chess::Square to = move.to();
    const chess::Piece piece = board.at(from);
    const chess::Piece to_piece = board.at(to);
    const int from_value = piece_value[static_cast<int>(piece.type())];

    chess::Bitboard attacked_by_pawn = 0;

    chess::Bitboard pawn_attackers = board.pieces(chess::PieceType::PAWN, ~board.sideToMove());

    while (pawn_attackers)
        attacked_by_pawn |= chess::attacks::pawn(~board.sideToMove(), pawn_attackers.pop());

    int score = 0;

    if ((piece != chess::Piece::WHITEKING) && (piece != chess::Piece::BLACKKING)){
        score += 100*psm.get_psm(piece, from, to);
    } else {
        bool is_endgame = board.occ().count() <= ENDGAME_PIECE_COUNT;
        score += 100*psm.get_ksm(piece, is_endgame, to, from);
    }
    
    if (piece.type() != chess::PieceType::PAWN){
        score += bool(attacked_by_pawn & chess::Bitboard::fromSquare(from))
                 * 40 * from_value * MATERIAL_CHANGE_MULTIPLIER;
        
        score -= bool(attacked_by_pawn & chess::Bitboard::fromSquare(to))
            * 41 * from_value * MATERIAL_CHANGE_MULTIPLIER;
    }

    // captures should be searched early, so
    // to_value = piece_value(to) - piece_value(from) doesn't seem to work.
    // however, find a way to make these captures even better ?
    if (to_piece != chess::Piece::NONE){
        score += 100 * piece_value[static_cast<int>(to_piece.type())] * MATERIAL_CHANGE_MULTIPLIER;
    }

    if (move.typeOf() == chess::Move::PROMOTION){
        score += 100 * piece_value[static_cast<int>(move.promotionType())] * MATERIAL_CHANGE_MULTIPLIER;
    }

    assert(depth != DEPTH_UNSEARCHED);
    if (killer_moves[depth].in_buffer(move)){
        score += 100 * KILLER_SCORE;
    }

    score += history.get_history_bonus(from.index(), to.index(), board.sideToMove() == chess::Color::WHITE); // cant be less than worst move score
    
    score = std::clamp(score, WORST_MOVE_SCORE + 1, BEST_MOVE_SCORE - 1);

    move.setScore(score);
}

template<>
void SortedMoveGen<chess::movegen::MoveGenType::CAPTURE>::set_score(chess::Move& move){
    move.setScore(piece_value[static_cast<int>(board.at(move.to()).type())] - 
                  piece_value[static_cast<int>(board.at(move.from()).type())]);
}

template<chess::movegen::MoveGenType MoveGenType>
void SortedMoveGen<MoveGenType>::set_tt_move(chess::Move move){
    tt_move = move;
}

template<>
bool SortedMoveGen<chess::movegen::MoveGenType::ALL>::is_valid_move(chess::Move move){
    return move != NO_MOVE;
}

template<>
bool SortedMoveGen<chess::movegen::MoveGenType::CAPTURE>::is_valid_move(chess::Move move){
    return move != NO_MOVE && board.isCapture(move);
}

template<chess::movegen::MoveGenType MoveGenType>
bool SortedMoveGen<MoveGenType>::next(chess::Move& move){
    move_idx++;

    if (checked_tt_move == false){
        checked_tt_move = true;
        if (is_valid_move(tt_move)){
            move = tt_move;
            return true;
        }
    }

    if (generated_moves == false){
        generated_moves = true;
        generate_moves();
        for (int i = 0; i < size_; i++){
            set_score(moves_[i]);
        }
        if (is_valid_move(tt_move)){
            pop_move(std::find(begin(), end(), tt_move) - begin());
        }
    }

    if (size() == 0)
        return false;

    move = pop_best_score();

    return true;
}

template<chess::movegen::MoveGenType MoveGenType>
chess::Move SortedMoveGen<MoveGenType>::pop_move(int move_idx){
    // to implement element removal from a movelist object,
    // the movelist is split into an unseen part first, and a seen part.

    // if the move is not in the last position, move it there.
    if (move_idx != size_-1){
        chess::Move swap = moves_[move_idx];
        moves_[move_idx] = moves_[size_-1];
        moves_[size_-1] = swap;
    }

    size_--;
    return moves_[size_];
}

template<chess::movegen::MoveGenType MoveGenType>
chess::Move SortedMoveGen<MoveGenType>::pop_best_score(){
    int score;
    int best_move_idx;
    int best_move_score = WORST_MOVE_SCORE;
    for (int i = 0; i < size_; i++){
        score = moves_[i].score();
        if (score > best_move_score){
            best_move_score = score;
            best_move_idx = i;
        }
    }

    return pop_move(best_move_idx);
}

template<chess::movegen::MoveGenType MoveGenType>
bool SortedMoveGen<MoveGenType>::is_empty(){ return empty(); }

template<chess::movegen::MoveGenType MoveGenType>
inline int SortedMoveGen<MoveGenType>::index(){ return move_idx; }

template<>
void SortedMoveGen<chess::movegen::MoveGenType::ALL>::clear_killer_moves(){
    std::fill(killer_moves.begin(), killer_moves.end(), CircularBuffer3());
}

template<>
void SortedMoveGen<chess::movegen::MoveGenType::ALL>::update_history(chess::Move best_move, int depth, bool color){
    int bonus = std::min(depth*depth*32 + 20, 1000);
    int idx = best_move.from().index()*64 + best_move.to().index();

    if (!board.isCapture(best_move))
        history.history[color][idx] += (bonus - history.history[color][idx] * std::abs(bonus) / MAX_HISTORY_BONUS);

    for (int i = 0; i < generated_moves_count; i++){
        if (moves_[i] != best_move && !board.isCapture(moves_[i])){
            idx = moves_[i].from().index()*64 + moves_[i].to().index();
            history.history[color][idx] += -bonus - history.history[color][idx] * std::abs(bonus) / MAX_HISTORY_BONUS;
        }
    }
}

template class SortedMoveGen<chess::movegen::MoveGenType::CAPTURE>;
template class SortedMoveGen<chess::movegen::MoveGenType::ALL>;
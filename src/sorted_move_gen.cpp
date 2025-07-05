#include "sorted_move_gen.hpp"

template<>
SortedMoveGen<movegen::MoveGenType::ALL>::SortedMoveGen(NnueBoard& pos, int depth): pos(pos), depth(depth) {};

template<>
SortedMoveGen<movegen::MoveGenType::CAPTURE>::SortedMoveGen(NnueBoard& pos): pos(pos) {};

template<>
void SortedMoveGen<movegen::MoveGenType::ALL>::prepare_pos_data(){
    const Color stm = pos.sideToMove();

    attacked_by_pawn = 0;

    Bitboard pawn_attackers = pos.pieces(PieceType::PAWN, ~stm);
    while (pawn_attackers)
        attacked_by_pawn |= attacks::pawn(~stm, pawn_attackers.pop());

    const Square opp_king_sq = pos.kingSq(~stm);
    const Bitboard occ = pos.occ();

    check_squares = {
        attacks::pawn(~stm, opp_king_sq), // pawn
        attacks::knight(opp_king_sq), // knight
        attacks::bishop(opp_king_sq, occ), // bishop
        attacks::rook(opp_king_sq, occ), // rook
        attacks::queen(opp_king_sq, occ), // queen
        0, // king
    };

    is_endgame = occ.count() <= 11;
}

// set move score to be sorted later
template<>
void SortedMoveGen<movegen::MoveGenType::ALL>::set_score(Move& move){
    
    const Color stm = pos.sideToMove();
    const Square from = move.from();
    const Square to = move.to();
    const Piece piece = pos.at(from);
    assert(piece.type() != PieceType::NONE);
    const Piece to_piece = pos.at(to);
    const int from_value = piece_value[static_cast<int>(piece.type())];

    int score = 0;

    if (piece != Piece::WHITEKING && piece != Piece::BLACKKING)
        score += psm.get_psm(piece, from, to);
    else
        score += psm.get_ksm(piece, is_endgame, to, from);
    
    if (piece.type() != PieceType::PAWN && piece.type() != PieceType::KING){
        score += 48 * bool(attacked_by_pawn & Bitboard::fromSquare(from)) * from_value/150;
        score -= 49 * bool(attacked_by_pawn & Bitboard::fromSquare(to)) * from_value/150;
    }

    if (check_squares[static_cast<int>(piece.type())] & Bitboard::fromSquare(to))
        score += 160;

    // captures should be searched early, so
    // to_value = piece_value(to) - piece_value(from) doesn't seem to work.
    // however, find a way to make these captures even better ?
    if (to_piece != Piece::NONE)
        score += 119 * piece_value[static_cast<int>(to_piece.type())]/150;

    if (move.typeOf() == Move::PROMOTION)
        score += 119 * piece_value[static_cast<int>(move.promotionType())]/150;

    assert(depth != DEPTH_UNSEARCHED);
    if (killer_moves[depth].in_buffer(move))
        score += 149;

    // cant be less than worst move score
    score += history.get_history_bonus(from.index(), to.index(), stm == Color::WHITE)/100;
    
    score = std::clamp(score, WORST_MOVE_SCORE + 1, BEST_MOVE_SCORE - 1);

    move.setScore(score);
}


template<>
void SortedMoveGen<movegen::MoveGenType::CAPTURE>::prepare_pos_data(){
    const Color stm = pos.sideToMove();
    const Square opp_king_sq = pos.kingSq(~stm);
    const Bitboard occ = pos.occ();

    check_squares = {
        attacks::pawn(~stm, opp_king_sq), // pawn
        attacks::knight(opp_king_sq), // knight
        attacks::bishop(opp_king_sq, occ), // bishop
        attacks::rook(opp_king_sq, occ), // rook
        attacks::queen(opp_king_sq, occ), // queen
        0, // king
    };
}

template<>
void SortedMoveGen<movegen::MoveGenType::CAPTURE>::set_score(Move& move){
    const int piece_type = static_cast<int>(pos.at(move.from()).type());
    const int to_piece_type = static_cast<int>(pos.at(move.to()).type());

    int score = piece_value[to_piece_type] - piece_value[piece_type]
        + 360 * bool(check_squares[piece_type] & Bitboard::fromSquare(move.to()));

    score = std::clamp(score, WORST_MOVE_SCORE + 1, BEST_MOVE_SCORE - 1);

    move.setScore(score);
}

template<movegen::MoveGenType MoveGenType>
void SortedMoveGen<MoveGenType>::set_tt_move(Move move){
    tt_move = move;
}

template<movegen::MoveGenType MoveGenType>
bool SortedMoveGen<MoveGenType>::next(Move& move){
    move_idx++;

    switch (stage){
        case TT_MOVE:
            ++stage;
            if (tt_move != Move::NO_MOVE && (MoveGenType == movegen::MoveGenType::ALL || pos.isCapture(tt_move))){
                move = tt_move;
                return true;
            }

        case GENERATE_MOVES:
            movegen::legalmoves<MoveGenType>(moves, pos);
            prepare_pos_data();
            for (int i = 0; i < moves.size(); i++){
                set_score(moves[i]);
            }
            if (tt_move != Move::NO_MOVE && (MoveGenType == movegen::MoveGenType::ALL || pos.isCapture(tt_move)))
                pop_move(std::find(moves.begin(), moves.end(), tt_move) - moves.begin());
            
            std::sort(moves.begin(), moves.end(),
                [](const Move& a, const Move& b) { return a.score() > b.score(); });
            ++stage;

        case GOOD_SEE:
            if (pop_best_see(move, SeeState::POSITIVE))
                return true;
            ++stage;

        case BAD_SEE:
            if (pop_best_see(move, SeeState::NEGATIVE))
                return true;
            break;
    }
    return false;
}

template<movegen::MoveGenType MoveGenType>
Move SortedMoveGen<MoveGenType>::pop_move(int move_idx){
    // to implement element removal from a movelist object,
    // the movelist is split into an unseen part first, and a seen part.

    // if the move is not in the last position, move it there.
    if (move_idx != moves.num_left-1){
        Move swap = moves[move_idx];
        moves[move_idx] = moves[moves.num_left-1];
        moves[moves.num_left-1] = swap;
    }

    return moves[--moves.num_left];
}

template<movegen::MoveGenType MoveGenType>
bool SortedMoveGen<MoveGenType>::pop_best_see(Move& move, SeeState threshold){
    if (moves.num_left == 0)
        return false;

    // find the best move that has good enough see.
    for (int i = 0; i < moves.num_left; i++){

        if (moves[i].see() == SeeState::NONE){
            bool above = SEE::evaluate(pos, moves[i], 0);
            moves[i].setSee(above ? SeeState::POSITIVE : SeeState::NEGATIVE);
        }

        if (moves[i].see() == threshold) {
            move = pop_move(i);
            return true;
        }
    }

    // if no moves have good see anymore, return false
    return false;
}

template<movegen::MoveGenType MoveGenType>
bool SortedMoveGen<MoveGenType>::empty(){ return moves.empty(); }

template<movegen::MoveGenType MoveGenType>
inline int SortedMoveGen<MoveGenType>::index(){ return move_idx; }

template<>
void SortedMoveGen<movegen::MoveGenType::ALL>::clear_killer_moves(){
    std::fill(killer_moves.begin(), killer_moves.end(), CircularBuffer3());
}

template<>
void SortedMoveGen<movegen::MoveGenType::ALL>::update_history(Move best_move, int depth, bool color){
    int bonus = std::min(depth*depth*32 + 20, 1000);
    int idx = best_move.from().index()*64 + best_move.to().index();

    if (!pos.isCapture(best_move))
        history.history[color][idx] += (bonus - history.history[color][idx] * std::abs(bonus) / MAX_HISTORY_BONUS);

    for (int i = 0; i < moves.size(); i++){
        if (moves[i] != best_move && !pos.isCapture(moves[i])){
            idx = moves[i].from().index()*64 + moves[i].to().index();
            history.history[color][idx] += -bonus - history.history[color][idx] * std::abs(bonus) / MAX_HISTORY_BONUS;
        }
    }
}

template class SortedMoveGen<movegen::MoveGenType::CAPTURE>;
template class SortedMoveGen<movegen::MoveGenType::ALL>;
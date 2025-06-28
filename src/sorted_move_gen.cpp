#include "sorted_move_gen.hpp"

template<>
SortedMoveGen<movegen::MoveGenType::ALL>::SortedMoveGen(NnueBoard& board, int depth): board(board), depth(depth) {};

template<>
SortedMoveGen<movegen::MoveGenType::CAPTURE>::SortedMoveGen(NnueBoard& board): board(board) {};

template<movegen::MoveGenType MoveGenType>
void SortedMoveGen<MoveGenType>::generate_moves(){
    movegen::legalmoves<MoveGenType>(*this, board);
    generated_moves_count = size_;
}

template<>
void SortedMoveGen<movegen::MoveGenType::ALL>::prepare_pos_data(){
    const Color stm = board.sideToMove();

    attacked_by_pawn = 0;

    Bitboard pawn_attackers = board.pieces(PieceType::PAWN, ~stm);
    while (pawn_attackers)
        attacked_by_pawn |= attacks::pawn(~stm, pawn_attackers.pop());

    const Square opp_king_sq = board.kingSq(~stm);
    const Bitboard occ = board.occ();

    check_squares = {
        attacks::pawn(~stm, opp_king_sq), // pawn
        attacks::knight(opp_king_sq), // knight
        attacks::bishop(opp_king_sq, occ), // bishop
        attacks::rook(opp_king_sq, occ), // rook
        attacks::queen(opp_king_sq, occ), // queen
    };

    is_endgame = occ.count() <= 11;
}

// set move score to be sorted later
template<>
void SortedMoveGen<movegen::MoveGenType::ALL>::set_score(Move& move){
    
    const Color stm = board.sideToMove();
    const Square from = move.from();
    const Square to = move.to();
    const Piece piece = board.at(from);
    assert(piece.type() != PieceType::NONE);
    const Piece to_piece = board.at(to);
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

    if (piece.type() != PieceType::KING
        && (check_squares[static_cast<int>(piece.type())] & Bitboard::fromSquare(to)))
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
    const Color stm = board.sideToMove();
    const Square opp_king_sq = board.kingSq(~stm);
    const Bitboard occ = board.occ();

    check_squares = {
        attacks::pawn(~stm, opp_king_sq), // pawn
        attacks::knight(opp_king_sq), // knight
        attacks::bishop(opp_king_sq, occ), // bishop
        attacks::rook(opp_king_sq, occ), // rook
        attacks::queen(opp_king_sq, occ), // queen
    };
}

template<>
void SortedMoveGen<movegen::MoveGenType::CAPTURE>::set_score(Move& move){
    const int piece_type = static_cast<int>(board.at(move.from()).type());
    const int to_piece_type = static_cast<int>(board.at(move.to()).type());

    int score = piece_value[to_piece_type] - piece_value[piece_type]
        + 360 * (piece_type != static_cast<int>(PieceType::KING)
                 && (check_squares[piece_type] & Bitboard::fromSquare(move.to())));

    score = std::clamp(score, WORST_MOVE_SCORE + 1, BEST_MOVE_SCORE - 1);

    move.setScore(score);
}

template<movegen::MoveGenType MoveGenType>
void SortedMoveGen<MoveGenType>::set_tt_move(Move move){
    tt_move = move;
}

template<>
bool SortedMoveGen<movegen::MoveGenType::ALL>::is_valid_move(Move move){
    return move != Move::NO_MOVE;
}

template<>
bool SortedMoveGen<movegen::MoveGenType::CAPTURE>::is_valid_move(Move move){
    return move != Move::NO_MOVE && board.isCapture(move);
}

template<movegen::MoveGenType MoveGenType>
bool SortedMoveGen<MoveGenType>::next(Move& move){
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
        prepare_pos_data();
        for (int i = 0; i < size_; i++){
            set_score(moves_[i]);
        }
        if (is_valid_move(tt_move))
            pop_move(std::find(begin(), end(), tt_move) - begin());
    }

    if (size() == 0)
        return false;

    move = pop_best_score();

    return true;
}

template<movegen::MoveGenType MoveGenType>
Move SortedMoveGen<MoveGenType>::pop_move(int move_idx){
    // to implement element removal from a movelist object,
    // the movelist is split into an unseen part first, and a seen part.

    // if the move is not in the last position, move it there.
    if (move_idx != size_-1){
        Move swap = moves_[move_idx];
        moves_[move_idx] = moves_[size_-1];
        moves_[size_-1] = swap;
    }

    size_--;
    return moves_[size_];
}

template<movegen::MoveGenType MoveGenType>
Move SortedMoveGen<MoveGenType>::pop_best_score(){
    int score;
    int best_move_idx;
    int best_move_score;
    while (true){
        best_move_score = WORST_MOVE_SCORE;
        for (int i = 0; i < size_; i++){
            score = moves_[i].score();
            if (score >= best_move_score){
                best_move_score = score;
                best_move_idx = i;
            }
        }
        if (best_move_score < -BAD_SEE_TRESHOLD || SEE::evaluate(board, moves_[best_move_idx], 0))
            break;
        
        moves_[best_move_idx].setScore(std::max(WORST_MOVE_SCORE, best_move_score - BAD_SEE_TRESHOLD));
    }

    return pop_move(best_move_idx);
}

template<movegen::MoveGenType MoveGenType>
bool SortedMoveGen<MoveGenType>::is_empty(){ return empty(); }

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

    if (!board.isCapture(best_move))
        history.history[color][idx] += (bonus - history.history[color][idx] * std::abs(bonus) / MAX_HISTORY_BONUS);

    for (int i = 0; i < generated_moves_count; i++){
        if (moves_[i] != best_move && !board.isCapture(moves_[i])){
            idx = moves_[i].from().index()*64 + moves_[i].to().index();
            history.history[color][idx] += -bonus - history.history[color][idx] * std::abs(bonus) / MAX_HISTORY_BONUS;
        }
    }
}

template class SortedMoveGen<movegen::MoveGenType::CAPTURE>;
template class SortedMoveGen<movegen::MoveGenType::ALL>;
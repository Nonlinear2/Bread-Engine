#include "sorted_move_gen.hpp"

TUNEABLE(endg, int, 11, 0, 32, 0.5, 0.002);
TUNEABLE(psm_2, int, 127, 0, 1000, 20, 0.002);
TUNEABLE(att_1, int, 46, 0, 500, 10, 0.002);
TUNEABLE(att_2, int, 54, 0, 500, 10, 0.002);
TUNEABLE(chk_1, int, 160, 0, 1000, 20, 0.002);
TUNEABLE(cpt, int, 153, 0, 1000, 20, 0.002);
TUNEABLE(prm, int, 102, 0, 1000, 20, 0.002);
TUNEABLE(kil, int, 134, 0, 1000, 25, 0.002);
TUNEABLE(his, int, 153, 0, 1000, 20, 0.002);
TUNEABLE(chis, int, 129, 0, 1000, 20, 0.002);
TUNEABLE(chk_2, int, 348, 0, 2000, 20, 0.002);
TUNEABLE(bst, int, 218, 0, 1500, 20, 0.002);

template<>
SortedMoveGen<movegen::MoveGenType::ALL>::SortedMoveGen(Movelist* to_search, Piece prev_piece, 
    Square prev_to, NnueBoard& pos, int depth):
    to_search(to_search), prev_piece(prev_piece), prev_to(prev_to), pos(pos), depth(depth) {};

template<>
SortedMoveGen<movegen::MoveGenType::CAPTURE>::SortedMoveGen(
    Piece prev_piece, Square prev_to, NnueBoard& pos): prev_piece(prev_piece), prev_to(prev_to), pos(pos) {};

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

    is_endgame = occ.count() <= endg;
}

// set move score to be sorted later
template<>
void SortedMoveGen<movegen::MoveGenType::ALL>::set_score(Move& move){
    
    const Color stm = pos.sideToMove();
    const Square from = move.from();
    const Square to = move.to();
    const Piece piece = pos.at(from);
    const Piece to_piece = pos.at(to);
    const int from_value = piece_value[static_cast<int>(piece.type())];
    assert(piece.type() != PieceType::NONE);

    int score = 0;

    if (piece == Piece::WHITEKING || piece == Piece::BLACKKING)
        score += psm_2 * psm.get_ksm(piece, is_endgame, to, from) / 100;
    
    if (piece.type() != PieceType::PAWN && piece.type() != PieceType::KING){
        score += att_1 * bool(attacked_by_pawn & Bitboard::fromSquare(from)) * from_value / 150;
        score -= att_2 * bool(attacked_by_pawn & Bitboard::fromSquare(to)) * from_value / 150;
    }

    if (check_squares[static_cast<int>(piece.type())] & Bitboard::fromSquare(to))
        score += chk_1;

    // captures should be searched early, so
    // to_value = piece_value(to) - piece_value(from) doesn't seem to work.
    // however, find a way to make these captures even better ?
    if (to_piece != Piece::NONE)
        score += cpt * piece_value[static_cast<int>(to_piece.type())] / 150;

    if (move.typeOf() == Move::PROMOTION)
        score += prm * piece_value[static_cast<int>(move.promotionType())] / 150;

    assert(depth != DEPTH_UNSEARCHED);
    if (killer_moves.in_buffer(depth, move))
        score += kil;

    // cant be less than worst move score
    score += his * history.get(stm == Color::WHITE, from.index(), to.index()) / 10'000;

    if (prev_piece != int(Piece::NONE) && prev_to != int(Square::underlying::NO_SQ))
        score += chis * cont_history.get(prev_piece, prev_to, piece, to.index()) / 10'000;

    score = std::clamp(score, WORST_MOVE_SCORE, BEST_MOVE_SCORE);

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
        + chk_2 * bool(check_squares[piece_type] & Bitboard::fromSquare(move.to()));

    score = std::clamp(score, WORST_MOVE_SCORE, BEST_MOVE_SCORE);

    move.setScore(score);
}

template<movegen::MoveGenType MoveGenType>
void SortedMoveGen<MoveGenType>::set_tt_move(Move move){
    tt_move = move;
}

template<movegen::MoveGenType MoveGenType>
bool SortedMoveGen<MoveGenType>::next(Move& move){
    move_idx++;
    if (to_search != NULL){
        if (move_idx == to_search->size())
            return false;
        move = (*to_search)[move_idx];
        return true;
    }

    switch (stage){
        case TT_MOVE:
            ++stage;

            use_tt_move = tt_move != Move::NO_MOVE && (
                MoveGenType == movegen::MoveGenType::ALL
                || pos.inCheck() 
                || pos.isCapture(tt_move)
            ) && pos.legal(tt_move);

            if (use_tt_move){
                move = tt_move;
                return true;
            }

        case GENERATE_MOVES:
            movegen::legalmoves<MoveGenType>(moves, pos);
            prepare_pos_data();
            for (int i = 0; i < moves.size(); i++){
                set_score(moves[i]);
                see[i] = SeeScore::UNSEEN;
                processed[i] = false;
            }
            ++stage;

        case GOOD_SEE:
            move = pop_best_score(SeeScore::GOOD);
            if (move != Move::NO_MOVE)
                return true;
            ++stage;

        case BAD_SEE:
            move = pop_best_score(SeeScore::BAD);
            if (move != Move::NO_MOVE)
                return true;
            ++stage;
    }
    return false;
}

template<movegen::MoveGenType MoveGenType>
Move SortedMoveGen<MoveGenType>::pop_best_score(SeeScore see_value){
    int best_idx;
    int best_score;
    while (true){
        best_score = WORST_MOVE_SCORE - 1;
        for (int i = 0; i < moves.size(); i++){
            if (moves[i] != tt_move && !processed[i] && (see[i] == SeeScore::UNSEEN || see[i] == see_value)){
                if (moves[i].score() >= best_score){
                    best_score = moves[i].score();
                    best_idx = i;
                }
            }
        }
        if (best_score == WORST_MOVE_SCORE - 1)
            return Move::NO_MOVE;

        if (see[best_idx] == SeeScore::UNSEEN)
            see[best_idx] = SEE::evaluate(pos, moves[best_idx], -bst) ? SeeScore::GOOD : SeeScore::BAD;

        if (see[best_idx] == see_value){
            processed[best_idx] = true;
            return moves[best_idx];
        }
    }
    return Move::NO_MOVE;
}

template<movegen::MoveGenType MoveGenType>
bool SortedMoveGen<MoveGenType>::empty(){ return moves.empty(); }

template<movegen::MoveGenType MoveGenType>
inline int SortedMoveGen<MoveGenType>::index(){ return move_idx; }

template<>
void SortedMoveGen<movegen::MoveGenType::ALL>::update_history(Move best_move, int bonus){
    bool color = pos.sideToMove() == Color::WHITE;

    history.apply_bonus(color, best_move.from(), best_move.to(), bonus);

    for (int i = 0; i < moves.size(); i++){
        if (moves[i] != best_move && !pos.isCapture(moves[i]))
            history.apply_bonus(color, moves[i].from(), moves[i].to(), -bonus);
    }
}

template<>
void SortedMoveGen<movegen::MoveGenType::ALL>::update_cont_history(Piece piece, Square to, int bonus){
    if (prev_piece != int(Piece::NONE) && prev_to != int(Square::underlying::NO_SQ))
        cont_history.apply_bonus(prev_piece, prev_to, piece, to, bonus);
}

template class SortedMoveGen<movegen::MoveGenType::CAPTURE>;
template class SortedMoveGen<movegen::MoveGenType::ALL>;
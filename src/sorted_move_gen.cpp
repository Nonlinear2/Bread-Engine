#include "sorted_move_gen.hpp"

UNACTIVE_TUNEABLE(endg, int, 11, 0, 32, 0.5, 0.002);
UNACTIVE_TUNEABLE(att_1, int, 46, 0, 500, 10, 0.002);
UNACTIVE_TUNEABLE(att_2, int, 54, 0, 500, 10, 0.002);
UNACTIVE_TUNEABLE(chk_1, int, 160, 0, 1000, 20, 0.002);
UNACTIVE_TUNEABLE(cpt, int, 153, 0, 1000, 20, 0.002);
UNACTIVE_TUNEABLE(prm, int, 102, 0, 1000, 20, 0.002);
UNACTIVE_TUNEABLE(kil, int, 134, 0, 1000, 25, 0.002);
UNACTIVE_TUNEABLE(his, int, 153, 0, 1000, 20, 0.002);
UNACTIVE_TUNEABLE(chis, int, 129, 0, 1000, 20, 0.002);
UNACTIVE_TUNEABLE(chk_2, int, 348, 0, 2000, 20, 0.002);
UNACTIVE_TUNEABLE(bst, int, 218, 0, 1500, 20, 0.002);
UNACTIVE_TUNEABLE(his_1, int, 34, 0, 300, 5, 0.002);
UNACTIVE_TUNEABLE(his_2, int, 33, 0, 300, 5, 0.002);
UNACTIVE_TUNEABLE(his_3, int, 1125, 0, 5000, 200, 0.002);
UNACTIVE_TUNEABLE(his_4, int, 34, 0, 300, 5, 0.002);
UNACTIVE_TUNEABLE(his_5, int, 33, 0, 300, 5, 0.002);
UNACTIVE_TUNEABLE(his_6, int, 1125, 0, 5000, 200, 0.002);

template<>
SortedMoveGen<GenType::NORMAL>::SortedMoveGen(Movelist* to_search, Piece prev_piece, 
    Square prev_to, NnueBoard& pos, int depth):
    to_search(to_search), prev_piece(prev_piece), prev_to(prev_to), pos(pos), depth(depth) {};

template<>
SortedMoveGen<GenType::QSEARCH>::SortedMoveGen(
    Piece prev_piece, Square prev_to, NnueBoard& pos): prev_piece(prev_piece), prev_to(prev_to), pos(pos) {};

template<>
void SortedMoveGen<GenType::NORMAL>::prepare_pos_data(){
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
}

template<>
void SortedMoveGen<GenType::QSEARCH>::prepare_pos_data(){
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

// set move score to be sorted later
template<>
void SortedMoveGen<GenType::NORMAL>::set_score(Move& move){
    if (stage == GENERATE_CAPTURES){
        const Color stm = pos.sideToMove();
        const Square from = move.from();
        const Square to = move.to();
        const Piece piece = pos.at(from);
        const Piece to_piece = pos.at(to);
        const int from_value = piece_value[piece.type()];
        assert(piece.type() != PieceType::NONE);

        int score = 0;

        if (check_squares[piece.type()] & Bitboard::fromSquare(to))
            score += chk_1;

        score += cpt * (piece_value[to_piece.type()] - from_value) / 150;

        if (move.typeOf() == Move::PROMOTION)
            score += prm * piece_value[move.promotionType()] / 150;

        assert(depth != DEPTH_UNSEARCHED);
        if (killer_moves.in_buffer(depth, move))
            score += kil;

        if (prev_piece != int(Piece::NONE) && prev_to != int(Square::underlying::NO_SQ))
            score += chis * cont_history.get(prev_piece, prev_to, piece, to.index()) / 10'000;

        score = std::clamp(score, WORST_MOVE_SCORE + 1, BEST_MOVE_SCORE - 1);

        move.setScore(score);
        return;

    } else if (stage == GENERATE_QUIETS){

        const Color stm = pos.sideToMove();
        const Square from = move.from();
        const Square to = move.to();
        const Piece piece = pos.at(from);
        const int from_value = piece_value[piece.type()];
        assert(piece.type() != PieceType::NONE);

        int score = 0;

        if (piece.type() != PieceType::PAWN && piece.type() != PieceType::KING){
            score += att_1 * bool(attacked_by_pawn & Bitboard::fromSquare(from)) * from_value / 150;
            score -= att_2 * bool(attacked_by_pawn & Bitboard::fromSquare(to)) * from_value / 150;
        }

        if (check_squares[piece.type()] & Bitboard::fromSquare(to))
            score += chk_1;

        if (move.typeOf() == Move::PROMOTION)
            score += prm * piece_value[move.promotionType()] / 150;

        assert(depth != DEPTH_UNSEARCHED);
        if (killer_moves.in_buffer(depth, move))
            score += kil;

        // cant be less than worst move score
        score += his * history.get(stm == Color::WHITE, from.index(), to.index()) / 10'000;

        if (prev_piece != int(Piece::NONE) && prev_to != int(Square::underlying::NO_SQ))
            score += chis * cont_history.get(prev_piece, prev_to, piece, to.index()) / 10'000;

        score = std::clamp(score, WORST_MOVE_SCORE + 1, BEST_MOVE_SCORE - 1);

        move.setScore(score);
        return;
    }
}

template<>
void SortedMoveGen<GenType::QSEARCH>::set_score(Move& move){
    const PieceType piece_type = pos.at(move.from()).type();
    const PieceType to_piece_type = pos.at(move.to()).type();

    int score = (to_piece_type == 6 ? -25000 : piece_value[to_piece_type]) - piece_value[piece_type]
        + chk_2 * bool(check_squares[piece_type] & Bitboard::fromSquare(move.to()));

    score = std::clamp(score, WORST_MOVE_SCORE + 1, BEST_MOVE_SCORE - 1);

    move.setScore(score);
}

template<GenType MoveGenType>
void SortedMoveGen<MoveGenType>::set_tt_move(Move move){
    tt_move = move;
}

template<GenType MoveGenType>
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

            if (tt_move != Move::NO_MOVE && pos.legal(tt_move)){
                move = tt_move;
                return true;
            }
            [[fallthrough]];

        case GENERATE_CAPTURES:
            movegen::legalmoves<movegen::MoveGenType::CAPTURE>(moves, pos);

            prepare_pos_data();
            for (int i = 0; i < moves.size(); i++)
                set_score(moves[i]);
            ++stage;
            [[fallthrough]];

        case GOOD_CAPTURES:
            while (moves.num_left != 0){
                move = pop_best_score(moves);
                if (move == tt_move)    
                    continue;

                if (SEE::evaluate(pos, move, -bst))
                    return true;
                else
                    bad_captures.add(move);
            }
            ++stage;
            if (!pos.inCheck() && MoveGenType == GenType::QSEARCH)
                stage = BAD_CAPTURES;
            [[fallthrough]];

        case GENERATE_QUIETS:
            movegen::legalmoves<movegen::MoveGenType::QUIET>(moves, pos);
            for (int i = 0; i < moves.size(); i++)
                set_score(moves[i]);
            ++stage;
            [[fallthrough]];

        case QUIETS:
            while (moves.num_left != 0){
                move = pop_best_score(moves);
                if (move != tt_move)    
                    return true;
            }
            ++stage;
            [[fallthrough]];

        case BAD_CAPTURES:
            while (bad_captures.num_left != 0){
                move = pop_move(bad_captures, 0);
                if (move != tt_move)    
                    return true;
            }
            break;
    }
    return false;
}

template<GenType MoveGenType>
Move SortedMoveGen<MoveGenType>::pop_move(Movelist& ml, int move_idx){
    // to implement element removal from a movelist object,
    // the movelist is split into an unseen part first, and a seen part.

    // if the move is not in the last position, move it there.
    if (move_idx != ml.num_left-1){
        Move swap = ml[move_idx];
        ml[move_idx] = ml[ml.num_left-1];
        ml[ml.num_left-1] = swap;
    }

    ml.num_left--;
    return ml[ml.num_left];
}

template<GenType MoveGenType>
Move SortedMoveGen<MoveGenType>::pop_best_score(Movelist& ml){
    int score;
    int best_move_idx = -1;
    int best_move_score = WORST_MOVE_SCORE;
    for (int i = 0; i < ml.num_left; i++){
        score = ml[i].score();
        if (score >= best_move_score){
            best_move_score = score;
            best_move_idx = i;
        }
    }
    assert(best_move_score == WORST_MOVE_SCORE || best_move_idx != -1);

    return pop_move(ml, best_move_idx);
}

template<GenType MoveGenType>
bool SortedMoveGen<MoveGenType>::empty(){ return moves.empty(); }

template<GenType MoveGenType>
inline int SortedMoveGen<MoveGenType>::index(){ return move_idx; }

template<>
void SortedMoveGen<GenType::NORMAL>::update_history(Move best_move, int depth){
    bool color = pos.sideToMove() == Color::WHITE;

    history.apply_bonus(color, best_move.from(), best_move.to(), std::min(depth*depth*his_1 + his_2, his_3));

    for (int i = moves.num_left; i < moves.size(); i++){
        if (moves[i] != best_move && !pos.isCapture(moves[i]))
            history.apply_bonus(color, moves[i].from(), moves[i].to(), -std::min(depth*depth*his_4 + his_5, his_6)/2);
    }
}

template<>
void SortedMoveGen<GenType::NORMAL>::update_cont_history(Piece prev_piece, Square prev_to, Piece piece, Square to, int bonus){
    if (prev_piece != int(Piece::NONE) && prev_to != int(Square::underlying::NO_SQ)
        && piece != int(Piece::NONE) && to != int(Square::underlying::NO_SQ))
        cont_history.apply_bonus(prev_piece, prev_to, piece, to, bonus);
}

template class SortedMoveGen<GenType::QSEARCH>;
template class SortedMoveGen<GenType::NORMAL>;
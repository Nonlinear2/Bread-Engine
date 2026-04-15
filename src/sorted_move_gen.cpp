#include "sorted_move_gen.hpp"

TUNEABLE(q_att_1, int, 92, 0, 500, 20, 0.002);
TUNEABLE(q_att_2, int, 85, 0, 500, 20, 0.002);
TUNEABLE(c_chk_1, int, 165, 0, 1000, 25, 0.002);
TUNEABLE(q_chk_1, int, 125, 0, 1000, 25, 0.002);
TUNEABLE(c_cpt, int, 288, 0, 1000, 60, 0.002);
TUNEABLE(c_prm, int, 189, 0, 1000, 40, 0.002);
TUNEABLE(q_prm, int, 145, 0, 1000, 30, 0.002);
TUNEABLE(q_kil, int, 136, 0, 1000, 25, 0.002);
TUNEABLE(q_his, int, 147, 0, 1000, 30, 0.002);
TUNEABLE(q_cthis, int, 144, 0, 1000, 30, 0.002);
TUNEABLE(bst, int, 213, 0, 1500, 40, 0.002);
TUNEABLE(his_1, int, 34, 0, 300, 5, 0.002);
TUNEABLE(his_2, int, 33, 0, 300, 5, 0.002);
TUNEABLE(his_3, int, 1049, 0, 5000, 200, 0.002);
TUNEABLE(his_4, int, 19, 0, 300, 3, 0.002);
TUNEABLE(his_5, int, 17, 0, 300, 3, 0.002);
TUNEABLE(his_6, int, 514, 0, 5000, 110, 0.002);
TUNEABLE(chis_1, int, 35, 0, 300, 5, 0.002);
TUNEABLE(chis_2, int, 36, 0, 300, 5, 0.002);
TUNEABLE(chis_3, int, 1103, 0, 5000, 200, 0.002);
TUNEABLE(chis_4, int, 17, 0, 300, 3, 0.002);
TUNEABLE(chis_5, int, 15, 0, 300, 3, 0.002);
TUNEABLE(chis_6, int, 431, 0, 5000, 110, 0.002);
TUNEABLE(cphis, int, 209, 0, 5000, 41, 0.002);


CaptureHistory capture_history = CaptureHistory();

template<>
SortedMoveGen<GenType::NORMAL>::SortedMoveGen(Movelist* to_search, Piece prev_piece, 
    Square prev_to, NnueBoard& pos, int depth):
    to_search(to_search), prev_piece(prev_piece), prev_to(prev_to), pos(pos), depth(depth) {};

template<>
SortedMoveGen<GenType::QSEARCH>::SortedMoveGen(
    Piece prev_piece, Square prev_to, NnueBoard& pos): prev_piece(prev_piece), prev_to(prev_to), pos(pos) {};

template<>
void SortedMoveGen<GenType::NORMAL>::prepare_capture_sort(){
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
void SortedMoveGen<GenType::NORMAL>::prepare_quiet_sort(){
    const Color stm = pos.sideToMove();

    Bitboard pawn_attackers = pos.pieces(PieceType::PAWN, ~stm);

    attacked_by_pawn = 0;
    while (pawn_attackers)
        attacked_by_pawn |= attacks::pawn(~stm, pawn_attackers.pop());
}

template<>
void SortedMoveGen<GenType::QSEARCH>::prepare_capture_sort(){}

template<>
void SortedMoveGen<GenType::QSEARCH>::prepare_quiet_sort(){}

// set move score to be sorted later
template<>
void SortedMoveGen<GenType::NORMAL>::set_score(Move& move){
    if (stage == GENERATE_CAPTURES){
        const Square from = move.from();
        const Square to = move.to();
        const Piece piece = pos.at(from);
        const Piece to_piece = pos.at(to);
        assert(piece.type() != PieceType::NONE);

        int score = 0;

        if (check_squares[piece.type()] & Bitboard::fromSquare(to))
            score += c_chk_1;

        score += c_cpt * piece_value[to_piece.type()] / 256;

        if (move.typeOf() == Move::PROMOTION)
            score += c_prm * piece_value[move.promotionType()] / 256;

        score += cphis * capture_history.get(piece, to.index(), to_piece) / 8192;

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
            score += q_att_1 * bool(attacked_by_pawn & Bitboard::fromSquare(from)) * from_value / 256;
            score -= q_att_2 * bool(attacked_by_pawn & Bitboard::fromSquare(to)) * from_value / 256;
        }

        if (check_squares[piece.type()] & Bitboard::fromSquare(to))
            score += q_chk_1;

        if (move.typeOf() == Move::PROMOTION)
            score += q_prm * piece_value[move.promotionType()] / 256;

        assert(depth != DEPTH_UNSEARCHED);
        if (killer_moves.in_buffer(depth, move))
            score += q_kil;

        // cant be less than worst move score
        score += q_his * history.get(stm, from.index(), to.index()) / 8192;

        if (prev_piece != int(Piece::NONE) && prev_to != int(Square::underlying::NO_SQ))
            score += q_cthis * cont_history.get(prev_piece, prev_to, piece, to.index()) / 8192;

        score = std::clamp(score, WORST_MOVE_SCORE + 1, BEST_MOVE_SCORE - 1);

        move.setScore(score);
        return;
    }
}

template<>
void SortedMoveGen<GenType::QSEARCH>::set_score(Move& move){

    int score = piece_value[pos.at(move.to()).type()];

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

            prepare_capture_sort();
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

            if (skip_quiets_)
                stage = BAD_CAPTURES;

            [[fallthrough]];

        case GENERATE_QUIETS:
            movegen::legalmoves<movegen::MoveGenType::QUIET>(moves, pos);

            prepare_quiet_sort();
            for (int i = 0; i < moves.size(); i++)
                set_score(moves[i]);
            ++stage;
            [[fallthrough]];

        case QUIETS:
            while (moves.num_left != 0){
                if (skip_quiets_)
                    break;

                move = pop_best_score(moves);
                if (move != tt_move)    
                    return true;
            }
            ++stage;
            [[fallthrough]];

        case BAD_CAPTURES:
             while (bad_capture_idx < bad_captures.size()){
                move = bad_captures[bad_capture_idx];
                bad_capture_idx++;
                if (move != tt_move)    
                    return true;
            }
            break;
    }
    return false;
}

template<GenType MoveGenType>
void SortedMoveGen<MoveGenType>::skip_quiets(){ skip_quiets_ = true; }

template<GenType MoveGenType>
Move SortedMoveGen<MoveGenType>::pop_move(Movelist& ml, int idx){
    // to implement element removal from a movelist object,
    // the movelist is split into an unseen part first, and a seen part.

    // if the move is not in the last position, move it there.
    if (idx != ml.num_left-1){
        Move swap = ml[idx];
        ml[idx] = ml[ml.num_left-1];
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
    history.apply_bonus(
        pos.sideToMove(), best_move.from(), 
        best_move.to(), std::min(depth*depth*his_1 + his_2, his_3)
    );

    for (int i = moves.num_left; i < moves.size(); i++){
        if (moves[i] != best_move && !pos.isCapture(moves[i]))
            history.apply_bonus(
                pos.sideToMove(), moves[i].from(), moves[i].to(), 
                -std::min(depth*depth*his_4 + his_5, his_6)
            );
    }
}

template<>
void SortedMoveGen<GenType::NORMAL>::update_capture_history(Move best_move, int depth){
    capture_history.apply_bonus(
        pos.at(best_move.from()), best_move.to(), 
        pos.at(best_move.to()), std::min(depth*depth*chis_1 + chis_2, chis_3));

    for (int i = moves.num_left; i < moves.size(); i++){
        if (moves[i] != best_move && pos.isCapture(moves[i]))
            capture_history.apply_bonus(
                pos.at(moves[i].from()), moves[i].to(),
                pos.at(moves[i].to()), -std::min(depth*depth*chis_4 + chis_5, chis_6)
            );
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
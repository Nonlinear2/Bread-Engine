#include "nnue_board.hpp"

NnueBoard::NnueBoard() {
    std::mt19937 rgen(rdev());
    std::uniform_int_distribution<int> idist(-BEST_VALUE, BEST_VALUE);
    synchronize();};

NnueBoard::NnueBoard(std::string_view fen) {
    std::mt19937 rgen(rdev());
    std::uniform_int_distribution<int> idist(-BEST_VALUE, BEST_VALUE);
    synchronize();
};

void NnueBoard::synchronize(){
    nnue_.compute_accumulator(get_HKP(true), true);
    nnue_.compute_accumulator(get_HKP(false), false);
}

bool NnueBoard::last_move_null(){
    return (prev_states_.back().hash == (hash()^Zobrist::sideToMove()));
}

void NnueBoard::update_state(Move move){
    // makeMove(move);
    accumulator_stack.push(nnue_.accumulator);

    if (is_updatable_move(move)){
        // white
        // modified_features mod_features = get_modified_features(move, true);
        // nnue_.update_accumulator(mod_features, true);
        // // black 
        // mod_features = get_modified_features(move, false);
        // nnue_.update_accumulator(mod_features, false);
        makeMove(move);
    } else {
        makeMove(move);
        // synchronize();
    }
}

void NnueBoard::restore_state(Move move){
    unmakeMove(move);

    // last layer accumulators will never be used with this implementation.
    nnue_.accumulator = accumulator_stack.top();
    accumulator_stack.pop();
}

int NnueBoard::evaluate(){
    return idist(rgen);
    // return std::clamp(nnue_.run_cropped_nn(sideToMove() == Color::WHITE), -BEST_VALUE, BEST_VALUE);
}

bool NnueBoard::try_outcome_eval(int& eval){
    // we dont want history dependent data to be stored in the TT.
    // the evaluation stored in the TT should only depend on the
    // position on the board and not how we got there, otherwise these evals would be reused
    // incorrectly.
    // here, threefold repetition and the fifty move rule are already handled and resulting evals
    // are not stored in the TT.
    if (isInsufficientMaterial()){
        eval = 0;
        return true;
    }

    Movelist movelist;
    movegen::legalmoves(movelist, *this);

    if (movelist.empty()){
        // checkmate/stalemate.
        eval = inCheck() ? -MATE_VALUE : 0;
        return true;
    }
    return false;
}

bool NnueBoard::probe_wdl(int& eval){
    if (occ().count() > TB_LARGEST){
        return false;
    }

    unsigned int ep_square = enpassantSq().index();
    if (ep_square == 64) ep_square = 0;

    unsigned int TB_hit = tb_probe_wdl(
            us(Color::WHITE).getBits(), us(Color::BLACK).getBits(), 
            pieces(PieceType::KING).getBits(), pieces(PieceType::QUEEN).getBits(),
            pieces(PieceType::ROOK).getBits(), pieces(PieceType::BISHOP).getBits(),
            pieces(PieceType::KNIGHT).getBits(), pieces(PieceType::PAWN).getBits(),
            halfMoveClock(), castlingRights().has(sideToMove()),
            ep_square, sideToMove() == Color::WHITE
    );
    switch(TB_hit){
        case TB_WIN:
            eval = TB_VALUE;
            return true;
        case TB_LOSS:
            eval = -TB_VALUE;
            return true;
        case TB_DRAW:
        case TB_CURSED_WIN:
        case TB_BLESSED_LOSS:
            eval = 0;
            return true;
        default:
            return false;
    }
}

bool NnueBoard::probe_root_dtz(Move& move, Movelist& moves, bool generate_moves){
    if (occ().count() > TB_LARGEST){
        return false;
    }

    unsigned int ep_square = enpassantSq().index();
    if (ep_square == 64) ep_square = 0;

    unsigned int tb_moves[TB_MAX_MOVES];

    unsigned int TB_hit = tb_probe_root(
            us(Color::WHITE).getBits(), us(Color::BLACK).getBits(), 
            pieces(PieceType::KING).getBits(), pieces(PieceType::QUEEN).getBits(),
            pieces(PieceType::ROOK).getBits(), pieces(PieceType::BISHOP).getBits(),
            pieces(PieceType::KNIGHT).getBits(), pieces(PieceType::PAWN).getBits(),
            halfMoveClock(), castlingRights().has(sideToMove()),
            ep_square, sideToMove() == Color::WHITE,
            generate_moves ? tb_moves : NULL
    );

    if ((TB_hit == TB_RESULT_CHECKMATE) || (TB_hit == TB_RESULT_STALEMATE) || (TB_hit == TB_RESULT_FAILED)){
        return false;
    }

    move = tb_result_to_move(TB_hit);

    if (generate_moves){
        Move current_move;
        for (int i = 0; i < TB_MAX_MOVES; i++){
            if (tb_moves[i] == TB_RESULT_FAILED) break;
            current_move = tb_result_to_move(tb_moves[i]);
            moves.add(current_move);
        }
    }
    return true;
}

Move NnueBoard::tb_result_to_move(unsigned int tb_result){
    Move move;
    if (TB_GET_PROMOTES(tb_result) == TB_PROMOTES_NONE){
        move = Move::make(
            static_cast<Square>(TB_GET_FROM(tb_result)),
            static_cast<Square>(TB_GET_TO(tb_result)));
    } else {
        PieceType promotion_type;

        switch (TB_GET_PROMOTES(tb_result)){
        case TB_PROMOTES_QUEEN:
            promotion_type = PieceType::QUEEN;
            break;
        case TB_PROMOTES_KNIGHT:
            promotion_type = PieceType::KNIGHT;
            break;
        case TB_PROMOTES_ROOK:
            promotion_type = PieceType::ROOK;
            break;
        case TB_PROMOTES_BISHOP:
            promotion_type = PieceType::BISHOP;
            break;
        }

        move = Move::make<Move::PROMOTION>(
            static_cast<Square>(TB_GET_FROM(tb_result)),
            static_cast<Square>(TB_GET_TO(tb_result)),
            promotion_type);
    }

    switch(TB_GET_WDL(tb_result)){
        case TB_WIN:
            move.setScore(TB_VALUE);
            break;
        case TB_LOSS:
            move.setScore(-TB_VALUE);
            break;
        default: // TB_DRAW, TB_CURSED_WIN, TB_BLESSED_LOSS
            move.setScore(0);
    }

    return move;
}

std::vector<int> NnueBoard::get_HKP(bool color){
    Bitboard occupied = occ();

    std::vector<int> active_features = std::vector<int>(occupied.count()-2);

    int king_square;

    int curr_piece;

    int idx = 0;
    while (occupied){
        int sq = occupied.pop();

        if (color){
            curr_piece = piece_to_index_w[static_cast<int>(at(static_cast<Square>(sq)))];
            sq = (63 - sq);
        } else {
            curr_piece = piece_to_index_b[static_cast<int>(at(static_cast<Square>(sq)))];
        }

        sq += 7 - 2 * (sq % 8); // reverse row

        if (curr_piece == 10){ // our king
            king_square = sq;
        } else if (curr_piece != 11) { // their king
            active_features[idx] = sq + curr_piece*64;
            idx++;
        }
    }
    for (int& feature: active_features){
        feature += king_square*640;
    }

    return active_features;
}


// assumes it is not a king move
// this function must be called before pushing the move
modified_features NnueBoard::get_modified_features(Move move, bool color){
    int king_square = kingSq(color ? Color::WHITE : Color::BLACK).index();
    
    int from;
    int to;
    int curr_piece_idx;
    int capt_piece_idx;

    int added;
    int removed;
    int captured = -1;

    from = move.from().index();
    to = move.to().index();

    Piece curr_piece = at(static_cast<Square>(from));
    Piece capt_piece = at(static_cast<Square>(to));

    if (color){
        king_square = 63 - king_square;
        from = 63 - from;
        to = 63 - to;
        curr_piece_idx = piece_to_index_w[static_cast<int>(curr_piece)];
    } else {
        curr_piece_idx = piece_to_index_b[static_cast<int>(curr_piece)];
    }

    king_square += 7 - 2 * (king_square % 8); // reverse row
    from += 7 - 2 * (from % 8); // reverse row
    to += 7 - 2 * (to % 8); // reverse row


    added = to + (curr_piece_idx + king_square * 10) * 64;
    
    removed = from + (curr_piece_idx + king_square * 10) * 64;

    if (capt_piece != Piece::NONE){
        if (color){
            capt_piece_idx = piece_to_index_w[static_cast<int>(capt_piece)];
        } else {
            capt_piece_idx = piece_to_index_b[static_cast<int>(capt_piece)];
        }
        captured = to + (capt_piece_idx + king_square * 10) * 64;
    }

    return modified_features(added, removed, captured);
}

bool NnueBoard::is_updatable_move(Move move){
    return ((move.typeOf() == Move::NORMAL) && (kingSq(sideToMove()) != move.from()));
}
#include "nnue_board.hpp"

NnueBoard::NnueBoard() {synchronize();};

NnueBoard::NnueBoard(std::string_view fen) {synchronize();};

void NnueBoard::synchronize(){
    nnue_.compute_accumulator(get_HKP(Color::WHITE), true);
    nnue_.compute_accumulator(get_HKP(Color::BLACK), false);
}

bool NnueBoard::last_move_null(){
    return (prev_states_.back().hash == (hash()^Zobrist::sideToMove()));
}

void NnueBoard::update_state(Move move){

    accumulator_stack.push(nnue_.accumulator);

    if (is_updatable_move(move)){
        // white
        modified_features mod_features = get_modified_features(move, Color::WHITE);
        nnue_.update_accumulator(mod_features, true);
        // black 
        mod_features = get_modified_features(move, Color::BLACK);
        nnue_.update_accumulator(mod_features, false);
        makeMove(move);
    } else {
        makeMove(move);
        synchronize();
    }
}

void NnueBoard::restore_state(Move move){
    unmakeMove(move);

    // last layer accumulators will never be used with this implementation.
    nnue_.accumulator = accumulator_stack.top();
    accumulator_stack.pop();
}

int NnueBoard::evaluate(){
    return std::clamp(nnue_.run_cropped_nn(sideToMove() == Color::WHITE), -BEST_VALUE, BEST_VALUE);
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

int hm_state_to_feature(int sq, int curr_piece, int king_sq){
    sq = ((sq >> 3) << 2) + (sq & 7); // equivalent to: sq = (sq / 8) * 4 + (sq % 8);
    return sq + curr_piece*32 + king_sq*320;
}

std::vector<int> NnueBoard::get_HKP(Color color){
    Bitboard occupied = occ();

    std::vector<int> active_features = std::vector<int>(occupied.count()-2);

    int king_sq = kingSq(color).index();

    
    bool mirror = king_sq & 0xF0F0F0F0F0F0F0F; // checks if the king is on the right side of the board

    int curr_piece;

    int idx = 0;
    while (occupied){
        int sq = occupied.pop();

        if (color == Color::WHITE)
            curr_piece = static_cast<int>(at(static_cast<Square>(sq)));
        else {
            curr_piece = static_cast<int>(at(static_cast<Square>(sq)));
            sq = 63 - sq; // flip board
            curr_piece = (curr_piece + 6) % 12; // flip piece color
        }

        if (mirror){
            // mirror horizontally by flipping last 3 bits.
            sq ^= 7;
            king_sq ^= 7;
        }

        assert(sq % 8 <= 4);

        if (curr_piece != 11 && curr_piece != 10) // their king / our king
            active_features[idx++] = hm_state_to_feature(sq, curr_piece, king_sq);
    }

    return active_features;
}


// assumes it is not a king move
// this function must be called before pushing the move
modified_features NnueBoard::get_modified_features(Move move, Color color){
    int king_sq = kingSq(color).index();
    
    int from = move.from().index();
    int to = move.to().index();
    int curr_piece = static_cast<int>(at(static_cast<Square>(from)));
    int capt_piece = static_cast<int>(at(static_cast<Square>(to)));

    int added;
    int removed;
    int captured = -1;

    if (color == Color::BLACK){
        // flip board
        king_sq = 63 - king_sq;
        from = 63 - from;
        to = 63 - to;
        // flip piece color
        curr_piece = (curr_piece + 6) % 12;
        capt_piece = capt_piece == 12 ? 12 : (capt_piece + 6) % 12;
    }

    added = hm_state_to_feature(to, curr_piece, king_sq);
    
    removed = hm_state_to_feature(from, curr_piece, king_sq);

    if (capt_piece != 12) // Piece::NONE
        captured = hm_state_to_feature(to, capt_piece, king_sq);

    return modified_features(added, removed, captured);
}

bool NnueBoard::is_updatable_move(Move move){
    return (move.typeOf() == Move::NORMAL && kingSq(sideToMove()) != move.from());
}
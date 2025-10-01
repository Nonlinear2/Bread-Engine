#include "nnue_board.hpp"

NnueBoard::NnueBoard() {synchronize();};

NnueBoard::NnueBoard(std::string_view fen) {synchronize();};

void NnueBoard::synchronize(){
    auto features = get_features();
    nnue_.compute_accumulator(features.first, true);
    nnue_.compute_accumulator(features.second, false);
}

void NnueBoard::update_state(Move move){

    accumulator_stack.push(nnue_.accumulator);

    if (is_updatable_move(move)){
        // white
        modified_features mod_features = get_modified_features(move, true);
        nnue_.update_accumulator(mod_features, true);
        // black 
        mod_features = get_modified_features(move, false);
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
    return std::clamp(nnue_.run_cropped_nn(sideToMove() == Color::WHITE, occ().count()), -BEST_VALUE, BEST_VALUE);
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

// const int piece_to_index_w[] = {
//     9, // white pawn
//     3, // white knight
//     5, // white bishop
//     1, // white rook
//     7, // white queen
//     10, // white king
//     8, // black pawn
//     2, // black knight
//     4, // black bishop
//     0, // black rook
//     6, // black queen
//     11, // black king
// };

// const int piece_to_index_b[] = {
//     8, // white pawn
//     2, // white knight
//     4, // white bishop
//     0, // white rook
//     6, // white queen
//     11, // white king
//     9, // black pawn
//     3, // black knight
//     5, // black bishop
//     1, // black rook
//     7, // black queen
//     10, // black king
// };

std::pair<std::vector<int>, std::vector<int>> NnueBoard::get_features(){
    Bitboard occupied = occ();

    std::vector<int> active_features_white = std::vector<int>(occupied.count());
    std::vector<int> active_features_black = std::vector<int>(occupied.count());

    Piece curr_piece;

    int idx = 0;
    while (occupied){
        int sq = occupied.pop();

        // let c = usize::from(piece & 8 > 0);
        // let pc = 64 * usize::from(piece & 7);
        // let sq = usize::from(square);

        // let stm = [0, 384][c] + pc + sq;
        // let ntm = [384, 0][c] + pc + (sq ^ 56);

        curr_piece = at(static_cast<Square>(sq));
        bool color = curr_piece.color() == Color::BLACK; // white: 0, black: 1
        int piece_idx = int(curr_piece.type());
        
        // white perspective
        active_features_white[idx] = 384 * color + piece_idx*64 + sq;
        // black perspective
        active_features_black[idx] = 384 * !color + piece_idx*64 + (sq ^ 56);
        idx++;
    }

    return std::make_pair(active_features_white, active_features_black);
}


// this function must be called before pushing the move
// it assumes it it not castling, en passant or a promotion
modified_features NnueBoard::get_modified_features(Move move, bool color){
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

    bool piece_color = curr_piece.color() == Color::BLACK; // white: 0, black: 1
    int piece_idx = int(curr_piece.type());

    if (color) { // white (yes this is confusing and inconsistent with piece color)
        added = 384 * piece_color + piece_idx*64 + to;
        removed = 384 * piece_color + piece_idx*64 + from;
    } else {
        added = 384 * !piece_color + piece_idx*64 + (to ^ 56);
        removed = 384 * !piece_color + piece_idx*64 + (from ^ 56);
    }

    if (capt_piece != Piece::NONE){
        bool capt_piece_color = capt_piece.color() == Color::BLACK; // white: 0, black: 1
        int capt_piece_idx = int(capt_piece.type());

        if (color)
            captured = 384 * capt_piece_color + capt_piece_idx*64 + to;
        else
            captured = 384 * !capt_piece_color + capt_piece_idx*64 + (to ^ 56);
    }

    return modified_features(added, removed, captured);
}

bool NnueBoard::is_updatable_move(Move move){
    return move.typeOf() == Move::NORMAL;
}
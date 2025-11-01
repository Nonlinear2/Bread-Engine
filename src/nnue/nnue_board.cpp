#include "nnue_board.hpp"

NnueBoard::NnueBoard() {
    accumulators_stack.push_empty();
    synchronize();
};

NnueBoard::NnueBoard(std::string_view fen) {
    accumulators_stack.push_empty();
    synchronize();
};

void NnueBoard::synchronize(){
    Accumulators& new_accs = accumulators_stack.top();
    auto features = get_features();
    nnue_.compute_accumulator(new_accs[(int)Color::WHITE], features.first);
    nnue_.compute_accumulator(new_accs[(int)Color::BLACK], features.second);
}

bool NnueBoard::legal(Move move){
    Piece piece = at(move.from());
    if (piece.color() != sideToMove())
        return false;

    Movelist legal;
    movegen::legalmoves(legal, *this, 1 << piece.type());

    return std::find(legal.begin(), legal.end(), move) != legal.end();
}

void NnueBoard::update_state(Move move){

    Accumulators& prev_accs = accumulators_stack.top();
    Accumulators& new_accs = accumulators_stack.push_empty();

    if (is_updatable_move(move)){
        // white
        modified_features mod_features = get_modified_features(move, Color::WHITE);
        nnue_.update_accumulator(prev_accs[(int)Color::WHITE], new_accs[(int)Color::WHITE], mod_features);

        // black
        mod_features = get_modified_features(move, Color::BLACK);
        nnue_.update_accumulator(prev_accs[(int)Color::BLACK], new_accs[(int)Color::BLACK], mod_features);

        makeMove(move);
    } else {
        makeMove(move);
        auto features = get_features();
        nnue_.compute_accumulator(new_accs[(int)Color::WHITE], features.first);
        nnue_.compute_accumulator(new_accs[(int)Color::BLACK], features.second);
    }
}

void NnueBoard::restore_state(Move move){
    unmakeMove(move);

    // last layer accumulators will never be used with this implementation.
    accumulators_stack.pop();
}

int NnueBoard::evaluate(){
    return std::clamp(nnue_.run(accumulators_stack.top(), sideToMove(), occ().count()), -BEST_VALUE, BEST_VALUE);
}

bool NnueBoard::try_outcome_eval(int& eval, int ply){
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
        eval = inCheck() ? pos_to_root_mate_value(-MATE_VALUE, ply) : 0;
        return true;
    }
    return false;
}

std::pair<std::vector<int>, std::vector<int>> NnueBoard::get_features(){
    Bitboard occupied = occ();

    std::vector<int> active_features_white = std::vector<int>(occupied.count());
    std::vector<int> active_features_black = std::vector<int>(occupied.count());

    Piece curr_piece;

    int idx = 0;
    while (occupied){
        int sq = occupied.pop();

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
modified_features NnueBoard::get_modified_features(Move move, Color color){
    assert(move != Move::NO_MOVE);

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
    assert(curr_piece != Piece::NONE);

    bool piece_color = curr_piece.color() == Color::BLACK; // white: 0, black: 1
    int piece_idx = int(curr_piece.type());

    if (color == Color::WHITE) {
        added = 384 * piece_color + piece_idx*64 + to;
        removed = 384 * piece_color + piece_idx*64 + from;
    } else {
        added = 384 * !piece_color + piece_idx*64 + (to ^ 56);
        removed = 384 * !piece_color + piece_idx*64 + (from ^ 56);
    }

    if (capt_piece != Piece::NONE){
        bool capt_piece_color = capt_piece.color() == Color::BLACK; // white: 0, black: 1
        int capt_piece_idx = int(capt_piece.type());

        if (color == Color::WHITE)
            captured = 384 * capt_piece_color + capt_piece_idx*64 + to;
        else
            captured = 384 * !capt_piece_color + capt_piece_idx*64 + (to ^ 56);
    }

    return modified_features(added, removed, captured);
}

bool NnueBoard::is_updatable_move(Move move){
    return move.typeOf() == Move::NORMAL;
}
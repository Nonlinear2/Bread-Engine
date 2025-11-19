#include "nnue_board.hpp"

NnueBoard::NnueBoard(){
    NNUE::init();
    accumulators_stack.push_empty();
    synchronize();
};

NnueBoard::NnueBoard(std::string_view fen){
    NNUE::init();
    accumulators_stack.push_empty();
    synchronize();
};

NnueBoard::~NnueBoard(){
    NNUE::cleanup();
};

void NnueBoard::synchronize(){
    Accumulators& new_accs = accumulators_stack.top();
    auto features = get_features();
    NNUE::compute_accumulator(new_accs[(int)Color::WHITE], features.first);
    NNUE::compute_accumulator(new_accs[(int)Color::BLACK], features.second);
    accumulators_stack.clear_top_update();
}

bool NnueBoard::legal(Move move){
    Piece piece = at(move.from());
    if (piece.color() != sideToMove())
        return false;

    Movelist legal;
    movegen::legalmoves(legal, *this, 1 << piece.type());

    return std::find(legal.begin(), legal.end(), move) != legal.end();
}

void NnueBoard::update_state(Move move, TranspositionTable& tt){

    Accumulators& new_accs = accumulators_stack.push_empty();

    if (is_updatable_move(move)){
        accumulators_stack.set_top_update(
            get_modified_features(move, Color::WHITE), 
            get_modified_features(move, Color::BLACK)
        );
        makeMove(move);
    } else {
        makeMove(move);
        auto features = get_features();
        NNUE::compute_accumulator(new_accs[(int)Color::WHITE], features.first);
        NNUE::compute_accumulator(new_accs[(int)Color::BLACK], features.second);
        accumulators_stack.clear_top_update();
    }
    __builtin_prefetch(&tt.entries[hash() & (tt.entries.size() - 1)]);
}

void NnueBoard::restore_state(Move move){
    unmakeMove(move);

    // last layer accumulators will never be used with this implementation.
    accumulators_stack.pop();
}

int NnueBoard::evaluate(){
    accumulators_stack.apply_lazy_updates();
    return std::clamp(NNUE::run(accumulators_stack.top(), sideToMove(), occ().count()), -BEST_VALUE, BEST_VALUE);
}

bool NnueBoard::is_stalemate(){
    Movelist movelist;
    movegen::legalmoves(movelist, *this);
    return movelist.empty();
}

std::pair<std::vector<int>, std::vector<int>> NnueBoard::get_features(){
    Bitboard occupied = occ();

    std::vector<int> active_features_white = std::vector<int>(occupied.count());
    std::vector<int> active_features_black = std::vector<int>(occupied.count());

    Piece curr_piece;

    bool mirror_w = kingSq(Color::WHITE).file() >= File::FILE_E;
    bool mirror_b = kingSq(Color::BLACK).file() >= File::FILE_E;

    int idx = 0;
    while (occupied){
        int sq = occupied.pop();

        curr_piece = at(static_cast<Square>(sq));
        bool color = curr_piece.color() == Color::BLACK; // white: 0, black: 1
        int piece_idx = int(curr_piece.type());
        
        // white perspective
        active_features_white[idx] = 384 * color + piece_idx*64 + (mirror_w ? (sq ^ 7) : sq);
        // black perspective
        active_features_black[idx] = 384 * !color + piece_idx*64 + (mirror_b ? ((sq ^ 56) ^ 7) : (sq ^ 56));
        idx++;
    }

    return std::make_pair(active_features_white, active_features_black);
}


// this function must be called before pushing the move
// it assumes it it not castling, en passant or a promotion
ModifiedFeatures NnueBoard::get_modified_features(Move move, Color color){
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

    if (kingSq(color).file() >= File::FILE_E){
        // mirror horizontally by flipping last 3 bits.
        from ^= 7;
        to ^= 7;
    }

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

    return ModifiedFeatures(added, removed, captured);
}

bool NnueBoard::is_updatable_move(Move move){
    return move.typeOf() == Move::NORMAL && (
        at(move.from()).type() != PieceType::KING
        || !((move.from().file() == File::FILE_D && move.to().file() == File::FILE_E)
            || (move.from().file() == File::FILE_E && move.to().file() == File::FILE_D)));
}

NnueBoard::AccumulatorsStack::AccumulatorsStack(){
    idx = 0;
}

Accumulators& NnueBoard::AccumulatorsStack::push_empty(){
    assert(idx < MAX_PLY + 1);
    return stack[++idx];
}

Accumulators& NnueBoard::AccumulatorsStack::top(){
    return stack[idx];
}

void NnueBoard::AccumulatorsStack::clear_top_update(){
    queued_updates[idx][0] = ModifiedFeatures();
    queued_updates[idx][1] = ModifiedFeatures();
}

void NnueBoard::AccumulatorsStack::set_top_update(ModifiedFeatures modified_white, ModifiedFeatures modified_black){
    queued_updates[idx][0] = modified_white;
    queued_updates[idx][1] = modified_black;
}

void NnueBoard::AccumulatorsStack::pop(){
    assert(idx > 0);
    clear_top_update();
    idx--;
}

void NnueBoard::AccumulatorsStack::apply_lazy_updates(){
    int i = idx;
    while (queued_updates[i][(int)Color::WHITE].added != 0)
        i--;

    Accumulators& prev_accs = stack[i];
    Accumulators& new_accs = stack[i + 1];

    NNUE::update_accumulators(prev_accs, &new_accs, &queued_updates[i + 1], idx - i);

    for (; i < idx; i++){
        queued_updates[i + 1][0] = ModifiedFeatures();
        queued_updates[i + 1][1] = ModifiedFeatures();
    }
}
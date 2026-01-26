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

    
    bool king_move = at(move.from()).type() == PieceType::KING;

    const bool crosses_middle =
        (move.from().file() == File::FILE_D && move.to().file() == File::FILE_E) ||
        (move.from().file() == File::FILE_E && move.to().file() == File::FILE_D);

    int flip = sideToMove() ? 56 : 0;

    if (move.typeOf() != Move::NORMAL){

        makeMove(move);
        auto features = get_features();
        NNUE::compute_accumulator(new_accs[(int)Color::WHITE], features.first);
        NNUE::compute_accumulator(new_accs[(int)Color::BLACK], features.second);
        accumulators_stack.clear_top_update();

    } else if (king_move && (crosses_middle || INPUT_BUCKETS[move.from().index() ^ flip] != INPUT_BUCKETS[move.to().index() ^ flip])){
        Color stm = sideToMove();
        
        ModifiedFeatures other_side_updates = stm == Color::WHITE 
            ? get_modified_features(move, Color::BLACK)
            : get_modified_features(move, Color::WHITE);

        makeMove(move);
        
        if (stm == Color::WHITE){
            NNUE::compute_accumulator(new_accs[(int)Color::WHITE], get_features(stm));
            accumulators_stack.set_top_update(
                ModifiedFeatures(),
                other_side_updates
            );
        } else {
            NNUE::compute_accumulator(new_accs[(int)Color::BLACK], get_features(stm));
            accumulators_stack.set_top_update(
                other_side_updates, 
                ModifiedFeatures()
            );
        }

    } else {
        accumulators_stack.set_top_update(
            get_modified_features(move, Color::WHITE), 
            get_modified_features(move, Color::BLACK)
        );
        makeMove(move);
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

    Square king_sq_w = kingSq(Color::WHITE);
    Square king_sq_b = kingSq(Color::BLACK);

    int mirror_w = king_sq_w.file() >= File::FILE_E ? 7 : 0;
    int mirror_b = king_sq_b.file() >= File::FILE_E ? 7 : 0;

    int idx = 0;
    while (occupied){
        int sq = occupied.pop();

        curr_piece = at(static_cast<Square>(sq));

        // white perspective
        active_features_white[idx] = 768 * INPUT_BUCKETS[king_sq_w.index()]
                                   + 384 * curr_piece.color() 
                                   + 64 * curr_piece.type() 
                                   + sq ^ mirror_w;
        // black perspective
        active_features_black[idx] = 768 * INPUT_BUCKETS[king_sq_b.index() ^ 56]
                                   + 384 * !curr_piece.color()
                                   + 64 * curr_piece.type()
                                   + sq ^ 56 ^ mirror_b;
        idx++;
    }

    return std::make_pair(active_features_white, active_features_black);
}

std::vector<int> NnueBoard::get_features(Color color){
    Bitboard occupied = occ();

    std::vector<int> active_features = std::vector<int>(occupied.count());

    Piece curr_piece;

    Square king_sq = kingSq(color);

    int mirror = king_sq.file() >= File::FILE_E ? 7 : 0;
    
    int flip = color ? 56 : 0; // mirror vertically by flipping bits 6, 5 and 4.

    int idx = 0;
    while (occupied){
        int sq = occupied.pop();

        curr_piece = at(static_cast<Square>(sq));

        active_features[idx] = 768 * INPUT_BUCKETS[king_sq.index() ^ flip]
                                   + 384 * (curr_piece.color() ^ color)
                                   + 64 * curr_piece.type()
                                   + (sq ^ flip ^ mirror);

        idx++;
    }

    return active_features;
}

// this function must be called before pushing the move
// it assumes it it not castling, en passant or a promotion
ModifiedFeatures NnueBoard::get_modified_features(Move move, Color color){
    assert(move != Move::NO_MOVE);

    int from = move.from().index();
    int to = move.to().index();

    int flip = color ? 56 : 0; // mirror vertically by flipping bits 6, 5 and 4.
    int mirror = kingSq(color).file() >= File::FILE_E ? 7 : 0; // mirror horizontally by flipping last 3 bits.

    int king_bucket = INPUT_BUCKETS[kingSq(color).index() ^ flip];

    Piece curr_piece = at(move.from());
    assert(curr_piece != Piece::NONE);

    int added = 768 * king_bucket + 384 * (curr_piece.color() ^ color) + 64 * curr_piece.type() + to ^ flip ^ mirror;
    int removed = 768 * king_bucket + 384 * (curr_piece.color() ^ color) + 64 * curr_piece.type() + from ^ flip ^ mirror;

    int captured = -1;

    Piece capt_piece = at(move.to());
    if (capt_piece != Piece::NONE)
        captured = 768 * king_bucket + 384 * (capt_piece.color() ^ color) + 64 * capt_piece.type() + to ^ flip ^ mirror;

    return ModifiedFeatures(added, removed, captured);
}

bool NnueBoard::is_updatable_move(Move move){
    if (move.typeOf() != Move::NORMAL)
        return false;

    if (at(move.from()).type() != PieceType::KING)
        return true;

    const bool crosses_middle =
        (move.from().file() == File::FILE_D && move.to().file() == File::FILE_E) ||
        (move.from().file() == File::FILE_E && move.to().file() == File::FILE_D);

    if (crosses_middle)
        return false;

    int flip = sideToMove() ? 56 : 0;
    return INPUT_BUCKETS[move.from().index() ^ flip] == INPUT_BUCKETS[move.to().index() ^ flip];
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
    int i_w = idx;
    int i_b = idx;
    while (queued_updates[i_w][(int)Color::WHITE].valid())
        i_w--;

    while (queued_updates[i_b][(int)Color::BLACK].valid())
        i_b--;

    int i = std::min(i_w, i_b);

    for (; i < idx; i++){
        Accumulators& prev_accs = stack[i];
        Accumulators& new_accs = stack[i + 1];

        // prefetch weight rows for black while processing white
        __builtin_prefetch(&NNUE::ft_weights[queued_updates[i + 1][1].added * ACC_SIZE]);
        __builtin_prefetch(&NNUE::ft_weights[queued_updates[i + 1][1].removed * ACC_SIZE]);

        // white
        if (i >= i_w){
            NNUE::update_accumulator(prev_accs[0], new_accs[0], queued_updates[i + 1][0]);
            queued_updates[i + 1][0] = ModifiedFeatures();
        }

        // black
        if (i >= i_b){
            NNUE::update_accumulator(prev_accs[1], new_accs[1], queued_updates[i + 1][1]);
            queued_updates[i + 1][1] = ModifiedFeatures();
        }
    }
}
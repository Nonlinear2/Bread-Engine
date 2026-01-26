#include "nnue_board.hpp"

AllBitboards::AllBitboards(){}

AllBitboards::AllBitboards(const NnueBoard& pos) {
    for (int color = 0; color < 2; color++)
        for (int pt = 0; pt < PIECETYPE_COUNT; pt++)
            bb[color][pt] = pos.pieces(
                PieceType(static_cast<PieceType::underlying>(pt)),
                static_cast<Color>(color)
            );
}

NnueBoard::NnueBoard(){
    NNUE::init();
    accumulators_stack.push_empty();
    synchronize();

    AllBitboards empty_pos = AllBitboards(); // empty position;
    Accumulator empty_acc;
    NNUE::compute_accumulator(empty_acc, {}); // accumulators for an empty position;
    Accumulators empty_accs = {empty_acc, empty_acc};
    for (int i = 0; i < INPUT_BUCKET_COUNT; i++){
        finny_table[i] = std::make_pair(empty_pos, empty_accs);
    }
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

    bool king_move = at(move.from()).type() == PieceType::KING;

    const bool crosses_middle =
        (move.from().file() == File::FILE_D && move.to().file() == File::FILE_E) ||
        (move.from().file() == File::FILE_E && move.to().file() == File::FILE_D);

    int flip = sideToMove() ? 56 : 0;

    if (move.typeOf() != Move::NORMAL || (king_move && crosses_middle)){
        Accumulators& new_accs = accumulators_stack.push_empty();

        makeMove(move);
        auto features = get_features();
        NNUE::compute_accumulator(new_accs[(int)Color::WHITE], features.first);
        NNUE::compute_accumulator(new_accs[(int)Color::BLACK], features.second);
        accumulators_stack.clear_top_update();

    } else if (king_move && INPUT_BUCKETS[move.from().index() ^ flip] != INPUT_BUCKETS[move.to().index() ^ flip]){
        accumulators_stack.apply_lazy_updates();

        finny_table[INPUT_BUCKETS[move.from().index() ^ flip]] = std::make_pair(
            AllBitboards(*this), accumulators_stack.top()
        );

        Accumulators& new_accs = accumulators_stack.push_empty();

        makeMove(move);

        auto [prev_pos, prev_accs] = finny_table[INPUT_BUCKETS[move.to().index() ^ flip]];

        auto features = get_features_difference(
            kingSq(Color::WHITE), kingSq(Color::BLACK), prev_pos, AllBitboards(*this)
        );

        NNUE::update_accumulator(prev_accs[(int)Color::WHITE], new_accs[(int)Color::WHITE], features.first);
        NNUE::update_accumulator(prev_accs[(int)Color::BLACK], new_accs[(int)Color::BLACK], features.second);

    } else {
        accumulators_stack.push_empty();

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

    std::vector<int> features_white = std::vector<int>(occupied.count());
    std::vector<int> features_black = std::vector<int>(occupied.count());

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
        features_white[idx] = 768 * INPUT_BUCKETS[king_sq_w.index()]
                                   + 384 * curr_piece.color() 
                                   + 64 * curr_piece.type() 
                                   + (sq ^ mirror_w);
        // black perspective
        features_black[idx] = 768 * INPUT_BUCKETS[king_sq_b.index() ^ 56]
                                   + 384 * !curr_piece.color()
                                   + 64 * curr_piece.type()
                                   + (sq ^ 56 ^ mirror_b);
        idx++;
    }

    return std::make_pair(features_white, features_black);
}


// this function must be called before pushing the move
// it assumes it it not castling, en passant or a promotion
ModifiedFeatures NnueBoard::get_modified_features(Move move, Color color){
    assert(move != Move::NO_MOVE);
    assert(pos.legal(move));

    int from = move.from().index();
    int to = move.to().index();

    int flip = color ? 56 : 0; // mirror vertically by flipping bits 6, 5 and 4.
    int mirror = kingSq(color).file() >= File::FILE_E ? 7 : 0; // mirror horizontally by flipping last 3 bits.

    int king_bucket = INPUT_BUCKETS[kingSq(color).index() ^ flip];

    Piece curr_piece = at(move.from());
    assert(curr_piece != Piece::NONE);

    int added = 768 * king_bucket + 384 * (curr_piece.color() ^ color) + 64 * curr_piece.type() + (to ^ flip ^ mirror);
    int removed = 768 * king_bucket + 384 * (curr_piece.color() ^ color) + 64 * curr_piece.type() + (from ^ flip ^ mirror);

    int captured = -1;

    Piece capt_piece = at(move.to());
    if (capt_piece != Piece::NONE)
        captured = 768 * king_bucket + 384 * (capt_piece.color() ^ color) + 64 * capt_piece.type() + (to ^ flip ^ mirror);

    return ModifiedFeatures(added, removed, captured);
}

std::pair<ModifiedFeaturesArray, ModifiedFeaturesArray> NnueBoard::get_features_difference(
    Square king_sq_w, Square king_sq_b, AllBitboards prev_pos, AllBitboards new_pos){

    int mirror_w = king_sq_w.file() >= File::FILE_E ? 7 : 0;
    int mirror_b = king_sq_b.file() >= File::FILE_E ? 7 : 0;

    ModifiedFeaturesArray features_white;
    features_white.added.reserve(32);
    features_white.removed.reserve(32);
    ModifiedFeaturesArray features_black;
    features_black.added.reserve(32);
    features_black.removed.reserve(32);

    for (int color = 0; color < 2; color++){
        for (int pt = 0; pt < PIECETYPE_COUNT; pt++){
            Bitboard added = new_pos.bb[color][pt] & (~prev_pos.bb[color][pt]);
            Bitboard removed = prev_pos.bb[color][pt] & (~new_pos.bb[color][pt]);

            while (added){
                int sq = added.pop();
                // white perspective
                features_white.added.push_back(
                    768 * INPUT_BUCKETS[king_sq_w.index()]
                                        + 384 * color
                                        + 64 * pt
                                        + (sq ^ mirror_w));
                // black perspective
                features_black.added.push_back(
                    768 * INPUT_BUCKETS[king_sq_b.index() ^ 56]
                                        + 384 * !color
                                        + 64 * pt
                                        + (sq ^ 56 ^ mirror_b));
            }

            while (removed){
                int sq = removed.pop();
                // white perspective
                features_white.removed.push_back(
                    768 * INPUT_BUCKETS[king_sq_w.index()]
                                        + 384 * color
                                        + 64 * pt
                                        + (sq ^ mirror_w));
                // black perspective
                features_black.removed.push_back(
                    768 * INPUT_BUCKETS[king_sq_b.index() ^ 56]
                                        + 384 * !color
                                        + 64 * pt
                                        + (sq ^ 56 ^ mirror_b));
            }
        }
    }

    return std::make_pair(features_white, features_black);
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
    while (queued_updates[i][(int)Color::WHITE].valid())
        i--;

    for (; i < idx; i++){
        Accumulators& prev_accs = stack[i];
        Accumulators& new_accs = stack[i + 1];

        // prefetch weight rows for black while processing white
        __builtin_prefetch(&NNUE::ft_weights[queued_updates[i + 1][1].added * ACC_SIZE]);
        __builtin_prefetch(&NNUE::ft_weights[queued_updates[i + 1][1].removed * ACC_SIZE]);

        // white
        NNUE::update_accumulator(prev_accs[0], new_accs[0], queued_updates[i + 1][0]);
        queued_updates[i + 1][0] = ModifiedFeatures();

        // black
        NNUE::update_accumulator(prev_accs[1], new_accs[1], queued_updates[i + 1][1]);
        queued_updates[i + 1][1] = ModifiedFeatures();
    }
}
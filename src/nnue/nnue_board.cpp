#include "nnue_board.hpp"

AllBitboards::AllBitboards(){
    for (int color = 0; color < 2; ++color)
        for (int pt = 0; pt < PIECETYPE_COUNT; ++pt)
            bb[color][pt] = Bitboard(0);
}

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

    AllBitboards empty_pos = AllBitboards(); // empty position;
    Accumulator empty_acc;
    NNUE::compute_accumulator(empty_acc, {}); // accumulators for an empty position;
    for (int bucket = 0; bucket < INPUT_BUCKET_COUNT; bucket++)
        for (int color = 0; color < 2; color++)
            for (int mirrored = 0; mirrored < 2; mirrored++)
                finny_table[bucket][color][mirrored] = std::make_pair(empty_pos, empty_acc);
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

    const bool queen_side_castle = move.typeOf() == Move::CASTLING && move.to() < move.from();
    const bool crosses_middle = king_move &&
        ((move.from().file() < File::FILE_E) != (move.to().file() < File::FILE_E)
        || queen_side_castle);

    int flip = sideToMove() ? 56 : 0;

    if (king_move && crosses_middle){
        Color stm = sideToMove();

        compute_top_update(move, ~stm);

        makeMove(move);

        NNUE::compute_accumulator(new_accs[(int)stm], get_features(stm));
        accumulators_stack.clear_top_update(stm);

    } else if (king_move && (INPUT_BUCKETS[move.from().index() ^ flip] != INPUT_BUCKETS[move.to().index() ^ flip])){
        Color stm = sideToMove();

        accumulators_stack.apply_lazy_updates();

        bool mirrored = kingSq(stm).file() >= File::FILE_E;

        finny_table[INPUT_BUCKETS[move.from().index() ^ flip]][stm][mirrored] = std::make_pair(
            AllBitboards(*this), accumulators_stack.top()[stm]
        );

        Accumulators& new_accs = accumulators_stack.push_empty();

        compute_top_update(move, ~stm);

        makeMove(move);
        int bucket = INPUT_BUCKETS[kingSq(stm).index() ^ flip];
        mirrored = kingSq(stm).file() >= File::FILE_E;

        auto [prev_pos, prev_acc] = finny_table[bucket][stm][mirrored];

        auto modified = get_modified_features(stm, mirrored ? 7 : 0, bucket, prev_pos, AllBitboards(*this));

        NNUE::update_accumulator(prev_acc, new_accs[stm], modified.first, modified.second);
        accumulators_stack.clear_top_update(stm);

    } else {
        Accumulators& new_accs = accumulators_stack.push_empty();
        compute_top_update(move, Color::WHITE);
        compute_top_update(move, Color::BLACK);
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

std::pair<Features, Features> NnueBoard::get_features(){
    Bitboard occupied = occ();

    Features active_features_white = Features(occupied.count());
    Features active_features_black = Features(occupied.count());

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
                       + (sq ^ mirror_w);
        // black perspective
        active_features_black[idx] = 768 * INPUT_BUCKETS[king_sq_b.index() ^ 56]
                       + 384 * !curr_piece.color()
                       + 64 * curr_piece.type()
                       + (sq ^ 56 ^ mirror_b);
        idx++;
    }

    return std::make_pair(active_features_white, active_features_black);
}

Features NnueBoard::get_features(Color color){
    Bitboard occupied = occ();

    Features active_features = Features(occupied.count());

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
void NnueBoard::compute_top_update(Move move, Color color){
    assert(move != Move::NO_MOVE);
    assert(legal(move));

    if (move.typeOf() == Move::CASTLING){
        assert(at<PieceType>(move.from()) == PieceType::KING);
        assert(at<PieceType>(move.to()) == PieceType::ROOK);

        const bool king_side = move.to() > move.from();

        int rook_from = move.to().index();
        int king_from = move.from().index();

        int rook_to = Square::castling_rook_square(king_side, sideToMove()).index();
        int king_to = Square::castling_king_square(king_side, sideToMove()).index();

        int flip = color ? 56 : 0; // mirror vertically by flipping bits 6, 5 and 4.
        int mirror = kingSq(color).file() >= File::FILE_E ? 7 : 0; // mirror horizontally by flipping last 3 bits.

        int king_bucket = INPUT_BUCKETS[kingSq(color).index() ^ flip];

        int added_king = 768 * king_bucket + 384 * (sideToMove() ^ color) + 64 * int(PieceType::KING) + (king_to ^ flip ^ mirror);
        int removed_king = 768 * king_bucket + 384 * (sideToMove() ^ color) + 64 * int(PieceType::KING) + (king_from ^ flip ^ mirror);

        int added_rook = 768 * king_bucket + 384 * (sideToMove() ^ color) + 64 * int(PieceType::ROOK) + (rook_to ^ flip ^ mirror);
        int removed_rook = 768 * king_bucket + 384 * (sideToMove() ^ color) + 64 * int(PieceType::ROOK) + (rook_from ^ flip ^ mirror);

        accumulators_stack.top_update()[color] = ModifiedFeatures(added_king, added_rook, removed_king, removed_rook);
        return;
    }

    assert(move.typeOf() != Move::CASTLING);

    int from = move.from().index();
    int to = move.to().index();

    int flip = color ? 56 : 0; // mirror vertically by flipping bits 6, 5 and 4.
    int mirror = kingSq(color).file() >= File::FILE_E ? 7 : 0; // mirror horizontally by flipping last 3 bits.

    int king_bucket = INPUT_BUCKETS[kingSq(color).index() ^ flip];

    PieceType piece_type = at<PieceType>(move.from());
    assert(piece_type != PieceType::NONE);

    int added = 768 * king_bucket
          + 384 * (sideToMove() ^ color)
          + 64 * (move.typeOf() == Move::PROMOTION ? move.promotionType() : piece_type)
          + (to ^ flip ^ mirror);

    int removed = 768 * king_bucket + 384 * (sideToMove() ^ color) + 64 * piece_type + (from ^ flip ^ mirror);
    int captured = -1;

    if (move.typeOf() == Move::ENPASSANT)
        captured = 768 * king_bucket + 384 * (~sideToMove() ^ color) + 64 * int(PieceType::PAWN) + (move.to().ep_square().index() ^ flip ^ mirror);
    else {
        PieceType capt_piece = at<PieceType>(move.to());
        if (capt_piece != PieceType::NONE)
            captured = 768 * king_bucket + 384 * (~sideToMove() ^ color) + 64 * capt_piece + (to ^ flip ^ mirror);
    }
    
    accumulators_stack.top_update()[color] = ModifiedFeatures(added, removed, captured);
    return;
}

std::pair<std::vector<int>, std::vector<int>> NnueBoard::get_modified_features(
    Color color, int mirror, int bucket, AllBitboards prev_pos, AllBitboards new_pos){

    int flip = color ? 56 : 0;

    std::vector<int> added_features = std::vector<int>();
    std::vector<int> removed_features = std::vector<int>();

    for (int pc = 0; pc < 2; pc++){
        for (int pt = 0; pt < PIECETYPE_COUNT; pt++){
            Bitboard added = new_pos.bb[pc][pt] & (~prev_pos.bb[pc][pt]);
            Bitboard removed = prev_pos.bb[pc][pt] & (~new_pos.bb[pc][pt]);

            while (added){
                int sq = added.pop();
                added_features.push_back(768 * bucket
                                        + 384 * (pc ^ color)
                                        + 64 * pt
                                        + (sq ^ flip ^ mirror));
            }

            while (removed){
                int sq = removed.pop();
                removed_features.push_back(768 * bucket
                                        + 384 * (pc ^ color)
                                        + 64 * pt
                                        + (sq ^ flip ^ mirror));
            }
        }
    }
    return std::make_pair(added_features, removed_features);
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

BothModifiedFeatures& NnueBoard::AccumulatorsStack::top_update(){
    return queued_updates[idx];
}

void NnueBoard::AccumulatorsStack::clear_top_update(){
    queued_updates[idx][0] = ModifiedFeatures();
    queued_updates[idx][1] = ModifiedFeatures();
}

void NnueBoard::AccumulatorsStack::clear_top_update(Color color){
    queued_updates[idx][color] = ModifiedFeatures();
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
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

    recompute_pawn_key();

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

    const bool crosses_middle = king_move &&
        ((move.from().file() < File::FILE_E) != (move.to().file() < File::FILE_E));

    const bool bucket_change = NNUE::input_bucket(move.from(), sideToMove()) != NNUE::input_bucket(move.to(), sideToMove());

    if (king_move && (move.typeOf() == Move::CASTLING || crosses_middle || bucket_change)){
        Color stm = sideToMove();

        Accumulators& new_accs = accumulators_stack.push_empty();

        compute_top_update(move, ~stm);

        makeMove(move);
        int bucket = NNUE::input_bucket(kingSq(stm), stm);
        int mirrored = kingSq(stm).file() >= File::FILE_E;

        auto [prev_pos, prev_acc] = finny_table[bucket][stm][mirrored];

        auto modified = get_modified_features(stm, prev_pos, *this);

        NNUE::update_accumulator(prev_acc, new_accs[stm], modified.first, modified.second);
        accumulators_stack.clear_top_update(stm);

        finny_table[bucket][stm][mirrored] = std::make_pair(
            AllBitboards(*this), accumulators_stack.top()[stm]
        );

    } else {
        Accumulators& new_accs = accumulators_stack.push_empty();
        compute_top_update(move, Color::WHITE);
        compute_top_update(move, Color::BLACK);
        makeMove(move);
    }

    __builtin_prefetch(&tt.entries[hash() & (tt.entries.size() - 1)]);

    uint16_t  pawn_k = get_pawn_key();
    recompute_pawn_key();
    assert(get_pawn_key() == pawn_k);
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

    int idx = 0;
    while (occupied){
        Square sq = occupied.pop();

        curr_piece = at(sq);

        // white perspective
        active_features_white[idx] = NNUE::feature(Color::WHITE, curr_piece, sq, king_sq_w);

        // black perspective
        active_features_black[idx] = NNUE::feature(Color::BLACK, curr_piece, sq, king_sq_b);

        idx++;
    }

    return std::make_pair(active_features_white, active_features_black);
}

Features NnueBoard::get_features(Color persp){
    Bitboard occupied = occ();

    Features active_features = Features(occupied.count());

    Square king_sq = kingSq(persp);

    int idx = 0;
    while (occupied){
        Square sq = occupied.pop();

        active_features[idx] = NNUE::feature(persp, at(sq), sq, king_sq);

        idx++;
    }

    return active_features;
}

// this function must be called before pushing the move
void NnueBoard::compute_top_update(Move move, Color persp){
    assert(move != Move::NO_MOVE);
    assert(legal(move));

    if (move.typeOf() == Move::CASTLING){
        assert(at<PieceType>(move.from()) == PieceType::KING);
        assert(at<PieceType>(move.to()) == PieceType::ROOK);

        const bool king_side = move.to() > move.from();

        Square rook_from = move.to();
        Square king_from = move.from();

        Square rook_to = Square::castling_rook_square(king_side, sideToMove());
        Square king_to = Square::castling_king_square(king_side, sideToMove());

        int added_king = NNUE::feature(persp, sideToMove(), PieceType::KING, king_to, kingSq(persp));

        int removed_king = NNUE::feature(persp, sideToMove(), PieceType::KING, king_from, kingSq(persp));

        int added_rook = NNUE::feature(persp, sideToMove(), PieceType::ROOK, rook_to, kingSq(persp));

        int removed_rook = NNUE::feature(persp, sideToMove(), PieceType::ROOK, rook_from, kingSq(persp));

        accumulators_stack.top_update()[persp] = ModifiedFeatures(added_king, added_rook, removed_king, removed_rook);
        return;
    }

    assert(move.typeOf() != Move::CASTLING);

    PieceType piece_type = at<PieceType>(move.from());
    assert(piece_type != PieceType::NONE);

    int added = NNUE::feature(persp, sideToMove(),
        move.typeOf() == Move::PROMOTION ? move.promotionType() : piece_type,
        move.to(), kingSq(persp));

    int removed = NNUE::feature(persp, sideToMove(), piece_type, move.from(), kingSq(persp));

    if (move.typeOf() == Move::ENPASSANT){
        int captured = NNUE::feature(persp, ~sideToMove(), PieceType::PAWN, move.to().ep_square(), kingSq(persp));
        accumulators_stack.top_update()[persp] = ModifiedFeatures(added, removed, captured);
        return;
    }
    if (at(move.to()) != Piece::NONE){
        int captured = NNUE::feature(persp, at(move.to()), move.to(), kingSq(persp));
        accumulators_stack.top_update()[persp] = ModifiedFeatures(added, removed, captured);
        return;
    }

    accumulators_stack.top_update()[persp] = ModifiedFeatures(added, removed);
    return;
}

std::pair<Features, Features> NnueBoard::get_modified_features(Color color, AllBitboards prev_pos, const NnueBoard& new_pos){

    Features added_features;
    Features removed_features;

    int added_idx = 0;
    int removed_idx = 0;

    Square king_sq = new_pos.kingSq(color);

    for (Color pc : {Color::WHITE, Color::BLACK}){
        for (PieceType pt : {
            PieceType::PAWN, PieceType::KNIGHT,
            PieceType::BISHOP, PieceType::ROOK,
            PieceType::QUEEN, PieceType::KING
        }){

            Bitboard new_bb = new_pos.pieces(pt, pc);
            Bitboard added = new_bb & (~prev_pos.bb[pc][pt]);
            Bitboard removed = prev_pos.bb[pc][pt] & (~new_bb);

            while (added){
                added_features[added_idx] = NNUE::feature(color, pc, pt, added.pop(), king_sq);
                added_idx++;
            }

            while (removed){
                removed_features[removed_idx] = NNUE::feature(color, pc, pt, removed.pop(), king_sq);
                removed_idx++;
            }
        }
    }
    added_features.size = added_idx;
    removed_features.size = removed_idx;

    return std::make_pair(added_features, removed_features);
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
        __builtin_prefetch(&NNUE::ft_weights[queued_updates[i + 1][1].added_1 * ACC_SIZE]);
        __builtin_prefetch(&NNUE::ft_weights[queued_updates[i + 1][1].removed_1 * ACC_SIZE]);

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
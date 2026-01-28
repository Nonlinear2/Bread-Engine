#include "nnue_board.hpp"


inline int NnueBoard::compute_feature_index(int king_bucket, Color piece_color, Color perspective_color, 
                                              PieceType piece_type, int square, int flip, int mirror) const {
    return 768 * king_bucket 
         + 384 * (piece_color ^ perspective_color)
         + 64 * piece_type
         + (square ^ flip ^ mirror);
}

inline std::pair<int, int> NnueBoard::get_flip_and_mirror(Color color) const {
    int flip = color ? 56 : 0; // mirror vertically by flipping bits 6, 5 and 4
    int mirror = kingSq(color).file() >= File::FILE_E ? 7 : 0; // mirror horizontally by flipping last 3 bits
    return {flip, mirror};
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
}

bool NnueBoard::legal(Move move){
    Piece piece = at(move.from());
    if (piece.color() != sideToMove())
        return false;

    Movelist legal;
    movegen::legalmoves(legal, *this, 1 << piece.type());

    return std::find(legal.begin(), legal.end(), move) != legal.end();
}

// void NnueBoard::update_state(Move move, TranspositionTable& tt){

//     Accumulators& new_accs = accumulators_stack.push_empty();

//     bool king_move = at(move.from()).type() == PieceType::KING;

//     const bool crosses_middle =
//         (move.from().file() == File::FILE_D && move.to().file() == File::FILE_E) ||
//         (move.from().file() == File::FILE_E && move.to().file() == File::FILE_D);

//     int flip = sideToMove() ? 56 : 0;

//     if (king_move && (crosses_middle || INPUT_BUCKETS[move.from().index() ^ flip] != INPUT_BUCKETS[move.to().index() ^ flip])){
//         Color stm = sideToMove();

//         ModifiedFeatures modified_features = get_modified_features(move, ~stm);

//         makeMove(move);

//         NNUE::compute_accumulator(new_accs[(int)stm], get_features(stm));
//         if (stm == Color::WHITE){
//             accumulators_stack.set_top_update(
//                 ModifiedFeatures(),
//                 modified_features
//             );
//         } else {
//             accumulators_stack.set_top_update(
//                 modified_features, 
//                 ModifiedFeatures()
//             );
//         }

//     } else {
//         accumulators_stack.set_top_update(
//             get_modified_features(move, Color::WHITE), 
//             get_modified_features(move, Color::BLACK)
//         );
//         makeMove(move);
//     }

//     __builtin_prefetch(&tt.entries[hash() & (tt.entries.size() - 1)]);
// }

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
        
        ModifiedFeatures modified_features = stm == Color::WHITE 
            ? get_modified_features(move, Color::BLACK)
            : get_modified_features(move, Color::WHITE);

        makeMove(move);
        
        if (stm == Color::WHITE){
            NNUE::compute_accumulator(new_accs[(int)Color::WHITE], get_features(stm));
            accumulators_stack.set_top_update(
                ModifiedFeatures(),
                modified_features
            );
        } else {
            NNUE::compute_accumulator(new_accs[(int)Color::BLACK], get_features(stm));
            accumulators_stack.set_top_update(
                modified_features, 
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


    auto [flip_w, mirror_w] = get_flip_and_mirror(Color::WHITE);
    auto [flip_b, mirror_b] = get_flip_and_mirror(Color::BLACK);

    int king_bucket_w = INPUT_BUCKETS[kingSq(Color::WHITE).index()];
    int king_bucket_b = INPUT_BUCKETS[kingSq(Color::BLACK).index() ^ 56];

    int idx = 0;
    while (occupied){
        int sq = occupied.pop();
        curr_piece = at(static_cast<Square>(sq));

        // white perspective
        active_features_white[idx] = compute_feature_index(king_bucket_w, curr_piece.color(), Color::WHITE, 
                                                            curr_piece.type(), sq, flip_w, mirror_w);
        // black perspective
        active_features_black[idx] = compute_feature_index(king_bucket_b, curr_piece.color(), Color::BLACK, 
                                                            curr_piece.type(), sq, flip_b, mirror_b);
        idx++;
    }

    return std::make_pair(active_features_white, active_features_black);
}

std::vector<int> NnueBoard::get_features(Color color){
    Bitboard occupied = occ();

    std::vector<int> active_features = std::vector<int>(occupied.count());

    Piece curr_piece;

    Square king_sq = kingSq(color);
    auto [flip, mirror] = get_flip_and_mirror(color);
    int king_bucket = INPUT_BUCKETS[king_sq.index() ^ flip];

    int idx = 0;
    while (occupied){
        int sq = occupied.pop();

        curr_piece = at(static_cast<Square>(sq));

        active_features[idx] = compute_feature_index(king_bucket, curr_piece.color(), color, 
                                                     curr_piece.type(), sq, flip, mirror);

        idx++;
    }

    return active_features;
}

// // this function must be called before pushing the move
// // it assumes it is not castling or a promotion
// ModifiedFeatures NnueBoard::get_modified_features(Move move, Color color){
//     assert(move != Move::NO_MOVE);
//     assert(legal(move));

//     if (move.typeOf() == Move::CASTLING){
//         assert(move.typeOf() == Move::CASTLING);
//         assert(at<PieceType>(move.from()) == PieceType::KING);
//         assert(at<PieceType>(move.to()) == PieceType::ROOK);

//         const bool king_side = move.to() > move.from();

//         int rook_from = move.to().index();
//         int king_from = move.from().index();

//         int rook_to    = Square::castling_rook_square(king_side, sideToMove()).index();
//         int king_to    = Square::castling_king_square(king_side, sideToMove()).index();

//         auto [flip, mirror] = get_flip_and_mirror(color);
//         int king_bucket = INPUT_BUCKETS[kingSq(color).index() ^ flip];

//         int added_king = compute_feature_index(king_bucket, sideToMove(), color, PieceType::KING, king_to, flip, mirror);
//         int removed_king = compute_feature_index(king_bucket, sideToMove(), color, PieceType::KING, king_from, flip, mirror);

//         int added_rook = compute_feature_index(king_bucket, sideToMove(), color, PieceType::ROOK, rook_to, flip, mirror);
//         int removed_rook = compute_feature_index(king_bucket, sideToMove(), color, PieceType::ROOK, rook_from, flip, mirror);

//         return ModifiedFeatures({added_king, added_rook}, {removed_king, removed_rook});
//     }

//     int from = move.from().index();
//     int to = move.to().index();

//     auto [flip, mirror] = get_flip_and_mirror(color);
//     int king_bucket = INPUT_BUCKETS[kingSq(color).index() ^ flip];

//     Piece curr_piece = at(move.from());
//     assert(curr_piece != Piece::NONE);

//     int added;
//     if (move.typeOf() == Move::PROMOTION)
//         added = compute_feature_index(king_bucket, sideToMove(), color, move.promotionType(), to, flip, mirror);
//     else
//         added = compute_feature_index(king_bucket, curr_piece.color(), color, curr_piece.type(), to, flip, mirror);

//     int removed = compute_feature_index(king_bucket, curr_piece.color(), color, curr_piece.type(), from, flip, mirror);

//     int captured = -1;
//     if (move.typeOf() == Move::ENPASSANT) {
//         captured = compute_feature_index(king_bucket, ~sideToMove(), color, PieceType::PAWN, 
//                                         move.to().ep_square().index(), flip, mirror);
//     } else {
//         Piece capt_piece = at(move.to());
//         if (capt_piece != Piece::NONE)
//             captured = compute_feature_index(king_bucket, capt_piece.color(), color, capt_piece.type(), to, flip, mirror);
//     }

//     return ModifiedFeatures(added, removed, captured);
// }

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
        if (queued_updates[i + 1][1].valid() && !queued_updates[i + 1][1].large_difference)
            __builtin_prefetch(&NNUE::ft_weights[queued_updates[i + 1][1].added * ACC_SIZE]);
        if (queued_updates[i + 1][1].valid() && !queued_updates[i + 1][1].large_difference)
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
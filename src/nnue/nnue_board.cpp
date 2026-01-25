#include "nnue_board.hpp"


AllBitboards AllBitboards::operator^(const AllBitboards& other) const {
    return AllBitboards(
        black_pawns ^ other.black_pawns, 
        white_pawns ^ other.white_pawns,
        black_knights ^ other.black_knights,
        white_knights ^ other.white_knights,
        black_bishops ^ other.black_bishops,
        white_bishops ^ other.white_bishops,
        black_rooks ^ other.black_rooks,
        white_rooks ^ other.white_rooks,
        black_queens ^ other.black_queens,
        white_queens ^ other.white_queens);
}

AllBitboards::AllBitboards(NnueBoard pos){
    black_pawns = pos.pieces(PieceType::PAWN, Color::BLACK);
    white_pawns = pos.pieces(PieceType::PAWN, Color::WHITE);

    black_knights = pos.pieces(PieceType::KNIGHT, Color::BLACK);
    white_knights = pos.pieces(PieceType::KNIGHT, Color::WHITE);

    black_bishops = pos.pieces(PieceType::BISHOP, Color::BLACK);
    white_bishops = pos.pieces(PieceType::BISHOP, Color::WHITE);

    black_rooks = pos.pieces(PieceType::ROOK, Color::BLACK);
    white_rooks = pos.pieces(PieceType::ROOK, Color::WHITE);

    black_queens = pos.pieces(PieceType::QUEEN, Color::BLACK);
    white_queens = pos.pieces(PieceType::QUEEN, Color::WHITE);   
}

AllBitboards::AllBitboards(
    Bitboard black_pawns, Bitboard white_pawns,
    Bitboard black_knights, Bitboard white_knights,
    Bitboard black_bishops, Bitboard white_bishops,
    Bitboard black_rooks, Bitboard white_rooks,
    Bitboard black_queens, Bitboard white_queens
): black_pawns(black_pawns), white_pawns(white_pawns),
black_knights(black_knights), white_knights(white_knights),
black_bishops(black_bishops), white_bishops(white_bishops),
black_rooks(black_rooks), white_rooks(white_rooks),
black_queens(black_queens), white_queens(white_queens) {}

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
        int flip = sideToMove() ? 56 : 0;
        // single move update
        if (INPUT_BUCKETS[move.from().index() ^ flip] == INPUT_BUCKETS[move.to().index() ^ flip])
            accumulators_stack.set_top_update(
                get_modified_features(move, Color::WHITE), 
                get_modified_features(move, Color::BLACK)
            );
        else { // finny tables update
            Accumulators last_accs = finny_table[move.to().index() ^ flip];
            
        }
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

    int added = 768 * king_bucket + 384 * (curr_piece.color() ^ color) + 64 * curr_piece.type() + to ^ flip ^ mirror;
    int removed = 768 * king_bucket + 384 * (curr_piece.color() ^ color) + 64 * curr_piece.type() + from ^ flip ^ mirror;

    int captured = -1;

    Piece capt_piece = at(move.to());
    if (capt_piece != Piece::NONE)
        captured = 768 * king_bucket + 384 * (capt_piece.color() ^ color) + 64 * capt_piece.type() + to ^ flip ^ mirror;

    return ModifiedFeatures(added, removed, captured);
}

std::pair<std::vector<int>, std::vector<int>> NnueBoard::get_features_from_difference(
    Square king_sq_w, Square king_sq_b, AllBitboards difference){

    int mirror_w = king_sq_w.file() >= File::FILE_E ? 7 : 0;
    int mirror_b = king_sq_b.file() >= File::FILE_E ? 7 : 0;
    
    std::vector<int> active_features_white = std::vector<int>();
    std::vector<int> active_features_black = std::vector<int>();

    for (Piece piece: {Piece::WHITEPAWN, Piece::BLACKPAWN}){
        while (difference[piece]){
            int sq = occupied.pop();
            // white perspective
            active_features_white.push_back(
                768 * INPUT_BUCKETS[king_sq_w.index()]
                                       + 384 * piece.color() 
                                       + 64 * piece.type() 
                                       + sq ^ mirror_w);
            // black perspective
            active_features_black.push_back(
                768 * INPUT_BUCKETS[king_sq_b.index() ^ 56]
                                       + 384 * !piece.color()
                                       + 64 * piece.type()
                                       + sq ^ 56 ^ mirror_b);
        }
    }

    return std::make_pair(active_features_white, active_features_black);
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

    return true;
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
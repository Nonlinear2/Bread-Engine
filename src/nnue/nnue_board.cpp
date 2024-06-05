#include "nnue_board.hpp"

NnueBoard::NnueBoard() {synchronize();};

NnueBoard::NnueBoard(std::string_view fen) {synchronize();};

void NnueBoard::synchronize(){
    nnue_.compute_accumulator(get_HKP(true), true);
    nnue_.compute_accumulator(get_HKP(false), false);
}

std::vector<int> NnueBoard::get_HKP(bool color){
    chess::Bitboard occupied = occ();

    std::vector<int> active_features = std::vector<int>(occupied.count()-2);

    int king_square;

    int curr_piece;

    int idx = 0;
    while (occupied){
        int sq = occupied.pop();

        if (color){
            curr_piece = piece_to_index_w[static_cast<int>(at(static_cast<chess::Square>(sq)))];
            sq = (63 - sq);
        } else {
            curr_piece = piece_to_index_b[static_cast<int>(at(static_cast<chess::Square>(sq)))];
        }

        sq += 7 - 2 * (sq % 8); // reverse row

        if (curr_piece == 10){ // our king
            king_square = sq;
        } else if (curr_piece != 11) { // their king
            active_features[idx] = sq + curr_piece*64;
            idx++;
        }
    }
    for (int& feature: active_features){
        feature += king_square*640;
    }

    return active_features;
}


// assumes it is not a king move
// this function must be called before pushing the move
modified_features NnueBoard::get_modified_features(chess::Move move, bool color){
    int king_square = kingSq(color ? chess::Color::WHITE : chess::Color::BLACK).index();
    
    int from;
    int to;
    int curr_piece_idx;
    int capt_piece_idx;

    int added;
    int removed;
    int captured = -1;

    from = move.from().index();
    to = move.to().index();

    chess::Piece curr_piece = at(static_cast<chess::Square>(from));
    chess::Piece capt_piece = at(static_cast<chess::Square>(to));

    if (color){
        king_square = 63 - king_square;
        from = 63 - from;
        to = 63 - to;
        curr_piece_idx = piece_to_index_w[static_cast<int>(curr_piece)];
    } else {
        curr_piece_idx = piece_to_index_b[static_cast<int>(curr_piece)];
    }

    king_square += 7 - 2 * (king_square % 8); // reverse row
    from += 7 - 2 * (from % 8); // reverse row
    to += 7 - 2 * (to % 8); // reverse row


    added = to + (curr_piece_idx + king_square * 10) * 64;
    
    removed = from + (curr_piece_idx + king_square * 10) * 64;

    if (capt_piece != chess::Piece::NONE){
        if (color){
            capt_piece_idx = piece_to_index_w[static_cast<int>(capt_piece)];
        } else {
            capt_piece_idx = piece_to_index_b[static_cast<int>(capt_piece)];
        }
        captured = to + (capt_piece_idx + king_square * 10) * 64;
    }

    return modified_features(added, removed, captured);
}

void NnueBoard::update_state(chess::Move move){

    accumulator_stack.push(nnue_.accumulator);

    bool basic_move = is_basic_move(move);

    if (basic_move){
        // white
        modified_features mod_features = get_modified_features(move, true);
        nnue_.update_accumulator(mod_features, true);
        // black 
        mod_features = get_modified_features(move, false);
        nnue_.update_accumulator(mod_features, false);
    }
    
    makeMove(move);

    if (!basic_move){
        synchronize();
    }
}

void NnueBoard::restore_state(chess::Move move){
    unmakeMove(move);

    // last layer accumulators will never be used with this implementation.
    nnue_.accumulator = accumulator_stack.top();
    accumulator_stack.pop();
}

bool NnueBoard::is_basic_move(chess::Move move){
    return ((move.typeOf() == chess::Move::NORMAL) & (kingSq(sideToMove()) != move.from()));
}

float NnueBoard::evaluate(){
    return nnue_.run_cropped_nn(sideToMove() == chess::Color::WHITE);
}
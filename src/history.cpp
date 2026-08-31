#include "history.hpp"

int16_t& ContinuationHistory::get(Piece prev_piece, Square prev_to, Piece piece, Square to){
    return this->history[
        prev_piece * NUM_SQUARES * NUM_PIECES * NUM_SQUARES
      + prev_to.index() * NUM_PIECES * NUM_SQUARES
      + piece * NUM_SQUARES
      + to.index()
    ];
}

int16_t& FromToHistory::get(Color color, Square from, Square to){
    return this->history[
        color * NUM_SQUARES * NUM_SQUARES
      + from.index() * NUM_SQUARES
      + to.index()
    ];
}

int16_t& CaptureHistory::get(Piece piece, Square to, Piece captured){
    // if the move was en passant, this function may be called with captured == None,
    // but idx will still be less than the history size
    int idx = piece * NUM_SQUARES * NUM_PIECETYPES 
            + to.index() * NUM_PIECETYPES 
            + static_cast<int>(captured.type());

    assert(idx < NUM_PIECES * NUM_SQUARES * NUM_PIECETYPES);
    return history[idx];
}

int16_t& PawnCorrectionHistory::get(Color color, uint16_t key){
    return this->history[NUM_COLORS * (key % PAWN_CORRHIST_SIZE) + color];
}

int16_t& MinorCorrectionHistory::get(Color color, uint16_t key){
    return this->history[NUM_COLORS * (key % MINOR_CORRHIST_SIZE) + color];
}

int16_t& MajorCorrectionHistory::get(Color color, uint16_t key){
    return this->history[NUM_COLORS * (key % MAJOR_CORRHIST_SIZE) + color];
}

int16_t& NonPawnCorrectionHistory::get(Color color, uint16_t key){
    return this->history[NUM_COLORS * (key % NONPAWN_CORRHIST_SIZE) + color];
}
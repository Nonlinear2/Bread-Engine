#include "history.hpp"

int& ContinuationHistory::get(Piece prev_piece, Square prev_to, Piece piece, Square to){
    return this->history[
        prev_piece * NUM_SQUARES * NUM_PIECES * NUM_SQUARES
      + prev_to.index() * NUM_PIECES * NUM_SQUARES
      + piece * NUM_SQUARES
      + to.index()
    ];
}

int& FromToHistory::get(Color color, Square from, Square to){
    return this->history[
        color * NUM_SQUARES * NUM_SQUARES
      + from.index() * NUM_SQUARES
      + to.index()
    ];
}

int& CaptureHistory::get(Piece piece, Square to, Piece captured){
    // if the move was en passant, this function may be called with captured == None,
    // but idx will still be less than the history size
    assert(piece*64*6 + to.index()*6 + static_cast<int>(captured.type()) < 12*64*6);
    return history[piece*64*6 + to.index()*6 + static_cast<int>(captured.type())];
}

int& PawnCorrectionHistory::get(Color color, uint16_t key){
    return this->history[NUM_COLORS*(key % PAWN_CORRHIST_SIZE) + color];
}

int& MinorCorrectionHistory::get(Color color, uint16_t key){
    return this->history[NUM_COLORS*(key % MINOR_CORRHIST_SIZE) + color];
}

int& MajorCorrectionHistory::get(Color color, uint16_t key){
    return this->history[NUM_COLORS*(key % MAJOR_CORRHIST_SIZE) + color];
}

int& NonPawnCorrectionHistory::get(Color color, uint16_t key){
    return this->history[NUM_COLORS*(key % NONPAWN_CORRHIST_SIZE) + color];
}
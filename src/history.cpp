#include "history.hpp"

UNACTIVE_TUNEABLE(CONTHIST_FILL_VALUE, int, -22, -5'000, 5'000, 50, 0.002);
UNACTIVE_TUNEABLE(HIST_FILL_VALUE, int, 2, -5'000, 5'000, 50, 0.002);
UNACTIVE_TUNEABLE(CAPTHIST_FILL_VALUE, int, -11, -5'000, 5'000, 50, 0.002);
UNACTIVE_TUNEABLE(MAX_CAPTHIST_BONUS, int, 9486, 0, 50'000, 2000, 0.002);
UNACTIVE_TUNEABLE(PAWN_CORRHIST_FILL_VALUE, int, 8, -5'000, 5'000, 50, 0.002);
UNACTIVE_TUNEABLE(MAX_CONTHIST_BONUS, int, 10462, 0, 50'000, 2000, 0.002);
UNACTIVE_TUNEABLE(MAX_HIST_BONUS, int, 9856, 0, 50'000, 2000, 0.002);
UNACTIVE_TUNEABLE(MAX_PAWN_CORRHIST_BONUS, int, 7955, 0, 50'000, 2000, 0.002);

void ContinuationHistory::clear(){
    std::fill(std::begin(history), std::end(history), CONTHIST_FILL_VALUE);
}

int& ContinuationHistory::get(Piece prev_piece, Square prev_to, Piece piece, Square to){
    return history[
        prev_piece * NUM_SQUARES * NUM_PIECES * NUM_SQUARES
      + prev_to.index() * NUM_PIECES * NUM_SQUARES
      + piece * NUM_SQUARES
      + to.index()
    ];
}

void ContinuationHistory::apply_bonus(Piece prev_piece, Square prev_to, Piece piece, Square to, int bonus){
    get(prev_piece, prev_to, piece, to) += bonus - get(prev_piece, prev_to, piece, to)
        * std::abs(bonus) / MAX_CONTHIST_BONUS;
}

void ContinuationHistory::save_to_stream(std::ofstream& ofs){
    for (const auto& v : history)
        ofs.write(reinterpret_cast<const char*>(&v), sizeof(int));
}

void ContinuationHistory::load_from_stream(std::ifstream& ifs){
    for (auto& v : history)
        ifs.read(reinterpret_cast<char*>(&v), sizeof(int));
}


void FromToHistory::clear(){
    std::fill(std::begin(history), std::end(history), HIST_FILL_VALUE);
}

int& FromToHistory::get(Color color, Square from, Square to){
    return history[
        color * NUM_SQUARES * NUM_SQUARES
      + from.index() * NUM_SQUARES
      + to.index()
    ];
}

void FromToHistory::apply_bonus(Color color, Square from, Square to, int bonus){
    get(color, from, to) += bonus - get(color, from, to) * std::abs(bonus) / MAX_HIST_BONUS;
}

void FromToHistory::save_to_stream(std::ofstream& ofs){
    for (const auto& v : history)
            ofs.write(reinterpret_cast<const char*>(&v), sizeof(int));
}

void FromToHistory::load_from_stream(std::ifstream& ifs){
    for (auto& v : history)
            ifs.read(reinterpret_cast<char*>(&v), sizeof(int));
}

void CaptureHistory::clear(){
    std::fill(std::begin(history), std::end(history), CAPTHIST_FILL_VALUE);
}

int& CaptureHistory::get(Piece piece, Square to, Piece captured){
    // if the move was en passant, this function may be called with captured == None,
    // but idx will still be less than the history size
    assert(piece*64*6 + to.index()*6 + static_cast<int>(captured.type()) < 12*64*6);
    return history[piece*64*6 + to.index()*6 + static_cast<int>(captured.type())];
}

void CaptureHistory::apply_bonus(Piece piece, Square to, Piece captured, int bonus){
    get(piece, to, captured) += bonus - get(piece, to, captured) * std::abs(bonus) / MAX_CAPTHIST_BONUS;
}

void CaptureHistory::save_to_stream(std::ofstream& ofs){
    for (const auto& v : history)
            ofs.write(reinterpret_cast<const char*>(&v), sizeof(int));
}

void PawnCorrectionHistory::clear(){
    std::fill(std::begin(history), std::end(history), PAWN_CORRHIST_FILL_VALUE);
}

int& PawnCorrectionHistory::get(Color color, uint16_t pawn_key){
    return history[NUM_COLORS*(pawn_key % PAWN_CORRHIST_SIZE) + color];
}

void PawnCorrectionHistory::apply_bonus(Color color, uint16_t pawn_key, int bonus){
    get(color, pawn_key) += bonus - get(color, pawn_key) * std::abs(bonus) / MAX_PAWN_CORRHIST_BONUS;
}

void PawnCorrectionHistory::save_to_stream(std::ofstream& ofs){
    for (const auto& v : history)
            ofs.write(reinterpret_cast<const char*>(&v), sizeof(int));
}

void CaptureHistory::load_from_stream(std::ifstream& ifs){
    for (auto& v : history)
        ifs.read(reinterpret_cast<char*>(&v), sizeof(int));
}

void PawnCorrectionHistory::load_from_stream(std::ifstream& ifs){
    for (auto& v : history)
            ifs.read(reinterpret_cast<char*>(&v), sizeof(int));
}
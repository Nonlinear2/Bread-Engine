#include "history.hpp"

UNACTIVE_TUNEABLE(MAX_CONTHIST_BONUS, int, 10'000, 0, 10'000, 2000, 0.002);
UNACTIVE_TUNEABLE(MAX_HIST_BONUS, int, 10'000, 0, 10'000, 2000, 0.002);

void ContinuationHistory::clear(){
    std::fill(std::begin(history), std::end(history), 0);
}

int& ContinuationHistory::get(Piece prev_piece, Square prev_to, Piece piece, Square to){
    return history[prev_piece * 64*12*64 + prev_to.index() * 12*64 + piece * 64 + to.index()];
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
    std::fill(std::begin(history), std::end(history), 0);
}

int& FromToHistory::get(bool color, Square from, Square to){
    return history[color*64*64 + from.index()*64 + to.index()];
}

void FromToHistory::apply_bonus(bool color, Square from, Square to, int bonus){
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


void PawnCorrectionHistory::clear(){
    std::fill(std::begin(history), std::end(history), 0);
}

int& PawnCorrectionHistory::get(bool color, uint16_t pawn_key){
    return history[2*pawn_key + color];
}

void PawnCorrectionHistory::apply_bonus(bool color, uint16_t pawn_key, int bonus){
    get(color, pawn_key) += bonus - get(color, pawn_key) * std::abs(bonus) / MAX_HISTORY_BONUS;
}

void PawnCorrectionHistory::save_to_stream(std::ofstream& ofs){
    for (const auto& v : history)
            ofs.write(reinterpret_cast<const char*>(&v), sizeof(int));
}

void PawnCorrectionHistory::load_from_stream(std::ifstream& ifs){
    for (auto& v : history)
            ifs.read(reinterpret_cast<char*>(&v), sizeof(int));
}
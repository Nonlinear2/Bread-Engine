#include "history.hpp"

void FromToHistory::clear(){
    std::fill(std::begin(history[0]), std::end(history[0]), 0);
    std::fill(std::begin(history[1]), std::end(history[1]), 0);
}

void ContinuationHistory::clear(){
    std::fill(std::begin(history), std::end(history), 0);
}

int& FromToHistory::get(bool color, Square from, Square to){
    return history[color][from.index()*64 + to.index()];
}

int& ContinuationHistory::get(Piece prev_piece, Square prev_to, Piece piece, Square to){
    return history[prev_piece * 64*12*64 + prev_to.index() * 12*64 + piece * 64 + to.index()];
}

void FromToHistory::apply_bonus(bool color, Square from, Square to, int bonus){
    get(color, from, to) += bonus - get(color, from, to) * std::abs(bonus) / MAX_HISTORY_BONUS;
}

void ContinuationHistory::apply_bonus(Piece prev_piece, Square prev_to, Piece piece, Square to, int bonus){
    get(prev_piece, prev_to, piece, to) += bonus - get(prev_piece, prev_to, piece, to)
        * std::abs(bonus) / MAX_HISTORY_BONUS;
}

void FromToHistory::save_to_stream(std::ofstream& ofs){
    for (const auto& row : history)
        for (const auto& v : row)
            ofs.write(reinterpret_cast<const char*>(&v), sizeof(int));
}

void ContinuationHistory::save_to_stream(std::ofstream& ofs){
    for (const auto& v : history)
        ofs.write(reinterpret_cast<const char*>(&v), sizeof(int));
}


void FromToHistory::load_from_stream(std::ifstream& ifs){
    for (auto& row : history)
        for (auto& v : row)
            ifs.read(reinterpret_cast<char*>(&v), sizeof(int));
}

void ContinuationHistory::load_from_stream(std::ifstream& ifs){
    for (auto& v : history)
        ifs.read(reinterpret_cast<char*>(&v), sizeof(int));
}
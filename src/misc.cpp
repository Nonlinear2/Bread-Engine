#include "misc.hpp"

// FIFO for killer moves
void KillerMoves::add_move(int depth, Move move){
    uint16_t val = move.move();
    if (moves[depth][0] != val && moves[depth][1] != val){
        moves[depth][2] = moves[depth][1];
        moves[depth][1] = moves[depth][0]; 
        moves[depth][0] = val;
    }
}

bool KillerMoves::in_buffer(int depth, Move move){
    uint16_t val = move.move();
    return moves[depth][0] == val || moves[depth][1] == val || moves[depth][2] == val;
}

void KillerMoves::clear(){
    std::fill(&moves[0][0], &moves[0][0] + sizeof(moves) / sizeof(uint16_t), 0);
}

void KillerMoves::save_to_stream(std::ofstream& ofs){
    for (const auto& row : moves)
        for (const auto& v : row)
            ofs.write(reinterpret_cast<const char*>(&v), sizeof(uint16_t));
}

void KillerMoves::load_from_stream(std::ifstream& ifs){
    for (auto& row : moves)
        for (auto& v : row)
            ifs.read(reinterpret_cast<char*>(&v), sizeof(uint16_t));
}

bool is_number_string(const std::string& s){
    std::string::const_iterator it = s.begin();
    while (it != s.end() && (std::isdigit(*it) || *it == '-')) ++it;
    return !s.empty() && it == s.end();
}
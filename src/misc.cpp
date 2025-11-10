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

AccumulatorsStack::AccumulatorsStack(){
    idx = 0;
}

Accumulators& AccumulatorsStack::push_empty(){
    assert(idx < MAX_PLY + 1);
    return stack[++idx];
}

Accumulators& AccumulatorsStack::top(){
    return stack[idx];
}

void AccumulatorsStack::pop(){
    assert(idx > 0);
    idx--;
}

int root_to_pos_mate_value(int value, int ply){
    assert(is_mate(value));
    assert(is_mate(std::abs(value) + ply)); // make sure new value is still mate
    return is_win(value) ? value + ply : value - ply;
}

int pos_to_root_mate_value(int value, int ply){
    assert(is_mate(value));
    assert(is_mate(std::abs(value) - ply)); // make sure new value is still mate
    return is_win(value) ? value - ply : value + ply;
}

int to_tt(int value, int ply){
    return is_mate(value) ? root_to_pos_mate_value(value, ply) : value;
}

// to make the engine prefer faster checkmates instead of stalling,
// we decrease the value if the checkmate is deeper in the search tree.
int get_mate_in_moves(int value){
    assert(is_mate(value));
    int ply = MATE_VALUE - std::abs(value);
    return (is_win(value) ? 1: -1)*(ply/2 + (ply%2 != 0));
}

bool is_number_string(const std::string& s){
    std::string::const_iterator it = s.begin();
    while (it != s.end() && (std::isdigit(*it) || *it == '-'))
        ++it;
    return !s.empty() && it == s.end();
}
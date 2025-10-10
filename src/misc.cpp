#include "misc.hpp"

TUNEABLE(p_v, int, 150, 0, 2000, 20, 0.002);
TUNEABLE(n_v, int, 450, 0, 2000, 25, 0.002);
TUNEABLE(b_v, int, 450, 0, 2000, 35, 0.002);
TUNEABLE(r_v, int, 750, 0, 5000, 50, 0.002);
TUNEABLE(q_v, int, 1350, 0, 8000, 100, 0.002);
TUNEABLE(k_v, int, 300, 0, 2000, 30, 0.002);

static std::vector<int> piece_value = {
    p_v, // pawn
    n_v, // knight
    b_v, // bishop
    r_v, // rook
    q_v, // queen
    k_v, // king
    0, // none
};

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

bool is_number_string(const std::string& s){
    std::string::const_iterator it = s.begin();
    while (it != s.end() && (std::isdigit(*it) || *it == '-'))
        ++it;
    return !s.empty() && it == s.end();
}
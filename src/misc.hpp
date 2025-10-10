#pragma once

#include <array>
#include <fstream>
#include <vector>
#include "piece_square_tables.hpp"
#include "constants.hpp"
#include "chess.hpp"
#include "tune.hpp"

TUNEABLE(p_v, int, 150, 0, 2000, 20, 0.002);
TUNEABLE(n_v, int, 450, 0, 2000, 25, 0.002);
TUNEABLE(b_v, int, 450, 0, 2000, 35, 0.002);
TUNEABLE(r_v, int, 750, 0, 5000, 50, 0.002);
TUNEABLE(q_v, int, 1350, 0, 8000, 100, 0.002);
TUNEABLE(k_v, int, 300, 0, 2000, 30, 0.002);

using namespace chess;

constexpr PieceSquareMaps psm = PieceSquareMaps();

const std::vector<int> piece_value = {
    p_v, // pawn
    n_v, // knight
    b_v, // bishop
    r_v, // rook
    q_v, // queen
    k_v, // king
    0, // none
};

struct Stack {
    Move excluded_move = Move::NO_MOVE;
    Move current_move = Move::NO_MOVE;
    Piece moved_piece = Piece::NONE;
    int static_eval = NO_VALUE;
};

class KillerMoves {
    public:
    uint16_t moves[ENGINE_MAX_DEPTH][3];

    void add_move(int depth, Move move);
    bool in_buffer(int depth, Move move);
    void clear();

    void save_to_stream(std::ofstream& ofs);
    void load_from_stream(std::ifstream& ifs);
};

enum LimitType {
    Time,
    Depth,
    Nodes,
};

class SearchLimit {
    public:
    SearchLimit() {};
    SearchLimit(LimitType type, int value): type(type), value(value) {};
    
    LimitType type;
    int value;
};

class AccumulatorsStack {
    public:
    AccumulatorsStack();
    Accumulators& push_empty();
    Accumulators& top();
    void pop();

    private:
    std::vector<Accumulators> stack = std::vector<Accumulators>(MAX_PLY + 1);
    int idx;
};

bool is_number_string(const std::string& s);

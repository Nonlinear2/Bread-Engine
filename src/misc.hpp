#pragma once

#include <array>
#include <fstream>
#include <vector>
#include "piece_square_tables.hpp"
#include "constants.hpp"
#include "chess.hpp"
#include "tune.hpp"

using namespace chess;

extern std::vector<int> piece_value;

constexpr PieceSquareMaps psm = PieceSquareMaps();

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

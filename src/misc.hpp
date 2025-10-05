#pragma once

#include <array>
#include <fstream>
#include "constants.hpp"
#include "chess.hpp"

using namespace chess;

const std::vector<int> piece_value = {
    150, // pawn
    450, // knight
    450, // bishop
    750, // rook
    1350, // queen
    300, // king
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
    std::array<Accumulators, MAX_PLY + 1> stack;
    int idx;
};

bool is_number_string(const std::string& s);

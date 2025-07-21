#pragma once

#include <array>
#include <fstream>
#include "chess.hpp"

using namespace chess;

constexpr int TT_MIN_SIZE = 2;
constexpr int TT_MAX_SIZE = 4096;

constexpr int ENGINE_MAX_DEPTH = 63;
constexpr int QSEARCH_MAX_DEPTH = 6;

constexpr int BENCHMARK_DEPTH = 9;

constexpr int DEPTH_UNSEARCHED = -1;
constexpr int DEPTH_QSEARCH = 0;

constexpr int BEST_MOVE_SCORE = 100'000;
constexpr int WORST_MOVE_SCORE = -100'000;
constexpr int BAD_SEE_TRESHOLD = 10'000;

constexpr int WORST_VALUE = -31'998;
constexpr int BEST_VALUE = 31'998;
constexpr int TB_VALUE = 31'999;
constexpr int MATE_VALUE = 32'600;
constexpr int NO_VALUE = 32'601;

constexpr int MAX_MATE_PLY = 1'00;
constexpr int INFINITE_VALUE = 32'700;
constexpr int MAX_HISTORY_BONUS = 10'000;
constexpr int SEE_KING_VALUE = 150'000;

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
};

class CircularBuffer3 {
    public:
    int curr_idx = 0;
    std::array<uint16_t, 3> data;

    void add_move(Move move);

    bool in_buffer(Move move);
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

class FromToHistory {
    public:
    void clear();
    int& get(bool color, int from, int to);
    void apply_bonus(bool color, int from, int to, int bonus);
    void save_to_stream(std::ofstream& ofs);
    void load_from_stream(std::ifstream& ifs);

    std::array<std::array<int, 64*64>, 2> history = {};
};

class ContinuationHistory {
    public:
    void clear();
    int& get(int prev_piece, int prev_to, int piece, int to);
    void apply_bonus(int prev_piece, int prev_to, int piece, int to, int bonus);
    void save_to_stream(std::ofstream& ofs);
    void load_from_stream(std::ifstream& ifs);

    std::array<int, 64*12*64*12> history = {};
};

namespace SEE {
bool evaluate(const Board& board, Move move, int threshold);
PieceType pop_lva(const Board& board, Bitboard& occupied, const Bitboard& attackers, Color color);
} // namespace SEE
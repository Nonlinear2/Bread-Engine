#pragma once

#include <array>
#include "chess.hpp"

#define NO_MOVE chess::Move::NO_MOVE

constexpr int ENGINE_MAX_DEPTH = 25;
constexpr int QSEARCH_MAX_DEPTH = 6;

constexpr int BENCHMARK_DEPTH = 9;

constexpr int DEPTH_UNSEARCHED = -1;
constexpr int DEPTH_QSEARCH = 0;

constexpr int BEST_MOVE_SCORE = 1'000'000;
constexpr int WORST_MOVE_SCORE = -1'000'000;

constexpr int WORST_VALUE = -31'998;
constexpr int BEST_VALUE = 31'998;
constexpr int TB_VALUE = 31'999;
constexpr int MATE_VALUE = 32'600;
constexpr int NO_VALUE = 32'601;

constexpr int MAX_MATE_PLY = 1'00;
constexpr int INFINITE_VALUE = 32'700;
constexpr int MAX_HISTORY_BONUS = 10'000;
constexpr int SEE_KING_VALUE = 1'000;

const std::vector<int> piece_value = {
    1, // pawn
    3, // knight
    3, // bishop
    5, // rook
    9, // queen
    2, // king
    0, // none
};

class CircularBuffer3 {
    public:
    int curr_idx = 0;
    std::array<uint16_t, 3> data;

    void add_move(chess::Move move);

    bool in_buffer(chess::Move move);
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

class History {
    public:
    void clear();
    int get_history_bonus(int from, int to, bool color);

    std::array<std::array<int, 64*64>, 2> history = {};
};

namespace SEE {
bool evaluate(const chess::Board& board, chess::Move move, int threshold);
chess::PieceType pop_lva(const chess::Board& board, chess::Bitboard& occupied, const chess::Bitboard& attackers, chess::Color color);
} // namespace SEE
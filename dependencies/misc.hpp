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

constexpr int MAX_MATE_PLY = 200;

constexpr int BEST_VALUE = 32'498;
constexpr int TB_VALUE = 32'499;
constexpr int MATE_VALUE = 32'700;
constexpr int INFINITE_VALUE = 32'701;
constexpr int NO_VALUE = 32'702;

constexpr int MAX_HISTORY_BONUS = 10'000;
constexpr int SEE_KING_VALUE = 150'000;

// |    value            |          name                    | is_valid  | is_mate   | is_decisive |
// ================================================================================================
// |       ...           |       xxx                        |   crash   | crash     | crash
// |      -32702         | -NO_VALUE                        |   false   | false     | crash
// |      -32701         | -INFINITE_VALUE                  |   false   | false     | crash
// |      -32700         | -MATE_VALUE (mate in 0)          |   true    | true      | true
// | -32699, ..., -32501 |       ...                        |   true    | true      | true
// |      -32500         | -MATE_VALUE+200 (mate in 200)    |   true    | true      | true
// |      -32499         | -TB_VALUE                        |   true    | false     | true

// |      -32498         | -BEST_VALUE                      |   true    | false     | false
// | -32498, ..., 32498  | valid eval range                 |   true    | false     | false
// |       32498         | BEST_VALUE                       |   true    | false     | false

// |       32499         | TB_VALUE                         |   true    | false     | true
// |       32500         | MATE_VALUE-200 (mate in 200)     |   true    | true      | true
// |  32501, ..., 32699  |       ...                        |   true    | true      | true
// |       32700         | MATE_VALUE (mate in 0)           |   true    | true      | true
// |       32701         | INFINITE_VALUE                   |   false   | false     | crash
// |       32702         | NO_VALUE                         |   false   | false     | crash
// |       ...           |       xxx                        |   crash   | crash     | crash

constexpr int is_valid(int value){
    assert(std::abs(value) <= NO_VALUE);
    return std::abs(value) < INFINITE_VALUE;
}

constexpr bool is_mate(int value){
    assert(std::abs(value) <= NO_VALUE);
    return (std::abs(value) >= MATE_VALUE - MAX_MATE_PLY && std::abs(value) <= MATE_VALUE);
}

constexpr bool is_win(int value){
    assert(is_valid(value));
    return value >= TB_VALUE;
}

constexpr bool is_loss(int value){
    assert(is_valid(value));
    return value <= -TB_VALUE;
}

constexpr bool is_decisive(int value){
    return is_win(value) || is_loss(value);
}

constexpr bool is_regular_eval(int value, bool zws_safe = true){
    assert(std::abs(value) <= NO_VALUE);
    return std::abs(value) <= BEST_VALUE - (zws_safe ? 1 : 0);
}

constexpr int increment_mate_ply(int value){
    assert(is_mate(value));
    assert(is_mate(std::abs(value) - 1)); // make sure new value is still mate
    return (is_win(value) ? 1 : -1)*(std::abs(value) - 1);
}

// to make the engine prefer faster checkmates instead of stalling,
// we decrease the value if the checkmate is deeper in the search tree.
constexpr int get_mate_in_moves(int value){
    assert(is_mate(value));
    int ply = MATE_VALUE - std::abs(value);
    return (is_win(value) ? 1: -1)*(ply/2 + (ply%2 != 0));
}

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

class ContinuationHistory {
    public:
    void clear();
    int& get(int prev_piece, int prev_to, int piece, int to);
    void apply_bonus(int prev_piece, int prev_to, int piece, int to, int bonus);
    void save_to_stream(std::ofstream& ofs);
    void load_from_stream(std::ifstream& ifs);

    std::array<int, 64*12*64*12> history = {};
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

namespace SEE {
bool evaluate(const Board& board, Move move, int threshold);
PieceType pop_lva(const Board& board, Bitboard& occupied, const Bitboard& attackers, Color color);
} // namespace SEE
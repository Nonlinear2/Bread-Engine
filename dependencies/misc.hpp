#pragma once

#include <array>
#include "chess.hpp"

#define NO_MOVE chess::Move::NO_MOVE
#define ENGINE_MAX_DEPTH 25

#define BEST_MOVE_SCORE 1000
#define WORST_MOVE_SCORE -1000

#define TB_EVAL 50'000

#define MATE_EVAL 90'000

#define WORST_EVAL -100'000
#define BEST_EVAL 100'000

#define MAX_HISTORY_BONUS 10'000

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
#pragma once

#include <array>
#include "chess.hpp"

#define NO_MOVE chess::Move::NO_MOVE
#define ENGINE_MAX_DEPTH 25

#define BEST_MOVE_SCORE 1000
#define WORST_MOVE_SCORE -1000

#define WORST_EVAL -100
#define BEST_EVAL 100

class CircularBuffer3 {
    public:
    int curr_idx = 0;
    std::array<uint16_t, 3> data;

    void add_move(chess::Move move);

    bool in_buffer(chess::Move move);
};

class TerminateSearch: public std::exception {};


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
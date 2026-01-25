#pragma once

#include <fstream>
#include <array>
#include "constants.hpp"
#include "tune.hpp"
#include "chess.hpp"

using namespace chess;

class ContinuationHistory {
    public:
    void clear();
    int& get(Piece prev_piece, Square prev_to, Piece piece, Square to);
    void apply_bonus(Piece prev_piece, Square prev_to, Piece piece, Square to, int bonus);
    void save_to_stream(std::ofstream& ofs);
    void load_from_stream(std::ifstream& ifs);

    std::array<int, 64*12*64*12> history = {};
};

class FromToHistory {
    public:
    void clear();
    int& get(bool color, Square from, Square to);
    void apply_bonus(bool color, Square from, Square to, int bonus);
    void save_to_stream(std::ofstream& ofs);
    void load_from_stream(std::ifstream& ifs);

    std::array<int, 2*64*64> history = {};
};
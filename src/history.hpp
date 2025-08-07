#pragma once

#include <fstream>
#include <array>
#include "constants.hpp"

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
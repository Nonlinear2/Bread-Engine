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

    std::array<int, 12*64*12*64> history = {};
};

class FromToHistory {
    public:
    void clear();
    int& get(Color color, Square from, Square to);
    void apply_bonus(Color color, Square from, Square to, int bonus);
    void save_to_stream(std::ofstream& ofs);
    void load_from_stream(std::ifstream& ifs);

    std::array<int, 2*64*64> history = {};
};

class CaptureHistory {
    public:
    void clear();
    int& get(Color color, Piece piece, Square to, Piece captured);
    void apply_bonus(Color color, Piece piece, Square to, Piece captured, int bonus);
    void save_to_stream(std::ofstream& ofs);
    void load_from_stream(std::ifstream& ifs);

    std::array<int, 2*12*64*12> history = {};
};

class PawnCorrectionHistory {
    public:
    void clear();
    int& get(Color color, uint16_t pawn_key);
    void apply_bonus(Color color, uint16_t pawn_key, int bonus);
    void save_to_stream(std::ofstream& ofs);
    void load_from_stream(std::ifstream& ifs);

    std::array<int, 2*PAWN_CORRHIST_SIZE> history = {};
};
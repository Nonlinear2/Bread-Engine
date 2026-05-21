#pragma once

#include <fstream>
#include <array>
#include "constants.hpp"
#include "tune.hpp"
#include "chess.hpp"

using namespace chess;

UNACTIVE_TUNEABLE(CONTHIST_FILL_VALUE, int, -22, -5'000, 5'000, 50, 0.002);
UNACTIVE_TUNEABLE(HIST_FILL_VALUE, int, 2, -5'000, 5'000, 50, 0.002);
UNACTIVE_TUNEABLE(CAPTHIST_FILL_VALUE, int, -11, -5'000, 5'000, 50, 0.002);
UNACTIVE_TUNEABLE(PAWN_CORRHIST_FILL_VALUE, int, 8, -5'000, 5'000, 50, 0.002);
UNACTIVE_TUNEABLE(MINOR_CORRHIST_FILL_VALUE, int, 8, -5'000, 5'000, 50, 0.002);
UNACTIVE_TUNEABLE(MAJOR_CORRHIST_FILL_VALUE, int, 8, -5'000, 5'000, 50, 0.002);
UNACTIVE_TUNEABLE(NONPAWN_CORRHIST_FILL_VALUE, int, 8, -5'000, 5'000, 50, 0.002);

UNACTIVE_TUNEABLE(MAX_CONTHIST_BONUS, int, 10462, 0, 50'000, 2000, 0.002);
UNACTIVE_TUNEABLE(MAX_HIST_BONUS, int, 9856, 0, 50'000, 2000, 0.002);
UNACTIVE_TUNEABLE(MAX_CAPTHIST_BONUS, int, 9486, 0, 50'000, 2000, 0.002);
UNACTIVE_TUNEABLE(MAX_PAWN_CORRHIST_BONUS, int, 7955, 0, 50'000, 2000, 0.002);
UNACTIVE_TUNEABLE(MAX_MINOR_CORRHIST_BONUS, int, 7955, 0, 50'000, 2000, 0.002);
UNACTIVE_TUNEABLE(MAX_MAJOR_CORRHIST_BONUS, int, 7955, 0, 50'000, 2000, 0.002);
UNACTIVE_TUNEABLE(MAX_NONPAWN_CORRHIST_BONUS, int, 7955, 0, 50'000, 2000, 0.002);

template <std::size_t size, int fill_value, int max_bonus>
class HistoryBase {
    public:
    HistoryBase() { clear(); }

    void clear(){
        std::fill(std::begin(history), std::end(history), fill_value);
    }

    void apply_bonus(int& value, int bonus){
        value += bonus - value * std::abs(bonus) / max_bonus;
    }

    void save_to_stream(std::ofstream& ofs){
        for (const auto& v : history)
            ofs.write(reinterpret_cast<const char*>(&v), sizeof(int));
    }

    void load_from_stream(std::ifstream& ifs){
        for (auto& v : history)
            ifs.read(reinterpret_cast<char*>(&v), sizeof(int));
    }

    std::array<int, size> history = {};
};

class ContinuationHistory: public HistoryBase<
        NUM_PIECES * NUM_SQUARES * NUM_PIECES * NUM_SQUARES,
        CONTHIST_FILL_VALUE,
        MAX_CONTHIST_BONUS
    > {
    public:
    int& get(Piece prev_piece, Square prev_to, Piece piece, Square to);
};

class FromToHistory: public HistoryBase<
        NUM_COLORS * NUM_SQUARES * NUM_SQUARES,
        HIST_FILL_VALUE,
        MAX_HIST_BONUS
    > {
    public:
    int& get(Color color, Square from, Square to);
};

class CaptureHistory: public HistoryBase<
        NUM_PIECES * NUM_SQUARES * NUM_PIECETYPES,
        CAPTHIST_FILL_VALUE,
        MAX_CAPTHIST_BONUS
    > {
    public:
    int& get(Piece piece, Square to, Piece captured);
};

class PawnCorrectionHistory: public HistoryBase<
        NUM_COLORS * PAWN_CORRHIST_SIZE,
        PAWN_CORRHIST_FILL_VALUE,
        MAX_PAWN_CORRHIST_BONUS
    > {
    public:
    int& get(Color color, uint16_t key);
};

class MinorCorrectionHistory: public HistoryBase<
        NUM_COLORS * MINOR_CORRHIST_SIZE,
        MINOR_CORRHIST_FILL_VALUE,
        MAX_MINOR_CORRHIST_BONUS
    > {
    public:
    int& get(Color color, uint16_t key);
};

class MajorCorrectionHistory: public HistoryBase<
        NUM_COLORS * MAJOR_CORRHIST_SIZE,
        MAJOR_CORRHIST_FILL_VALUE,
        MAX_MAJOR_CORRHIST_BONUS
    > {
    public:
    int& get(Color color, uint16_t key);
};

class NonPawnCorrectionHistory: public HistoryBase<
        NUM_COLORS * NONPAWN_CORRHIST_SIZE,
        NONPAWN_CORRHIST_FILL_VALUE,
        MAX_NONPAWN_CORRHIST_BONUS
    > {
    public:
    int& get(Color color, uint16_t key);
};
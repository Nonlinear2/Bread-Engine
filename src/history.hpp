#pragma once

#include <fstream>
#include <array>
#include "constants.hpp"
#include "tune.hpp"
#include "chess.hpp"

using namespace chess;

UNACTIVE_TUNEABLE(CONTHIST_FILL_VALUE, int, -22, -5'000, 5'000, 4, 0.002);
UNACTIVE_TUNEABLE(HIST_FILL_VALUE, int, 2, -5'000, 5'000, 2, 0.002);
UNACTIVE_TUNEABLE(CAPTHIST_FILL_VALUE, int, -12, -5'000, 5'000, 3, 0.002);
UNACTIVE_TUNEABLE(PAWN_CORRHIST_FILL_VALUE, int, 9, -5'000, 5'000, 3, 0.002);
UNACTIVE_TUNEABLE(MINOR_CORRHIST_FILL_VALUE, int, 8, -5'000, 5'000, 3, 0.002);
UNACTIVE_TUNEABLE(MAJOR_CORRHIST_FILL_VALUE, int, 10, -5'000, 5'000, 3, 0.002);
UNACTIVE_TUNEABLE(NONPAWN_CORRHIST_FILL_VALUE, int, 9, -5'000, 5'000, 3, 0.002);

UNACTIVE_TUNEABLE(MAX_CONTHIST_BONUS, int, 10790, 0, 50'000, 2000, 0.002);
UNACTIVE_TUNEABLE(MAX_HIST_BONUS, int, 10714, 0, 50'000, 2000, 0.002);
UNACTIVE_TUNEABLE(MAX_CAPTHIST_BONUS, int, 8833, 0, 50'000, 2000, 0.002);
UNACTIVE_TUNEABLE(MAX_PAWN_CORRHIST_BONUS, int, 8413, 0, 50'000, 1600, 0.002);
UNACTIVE_TUNEABLE(MAX_MINOR_CORRHIST_BONUS, int, 8662, 0, 50'000, 1600, 0.002);
UNACTIVE_TUNEABLE(MAX_MAJOR_CORRHIST_BONUS, int, 8286, 0, 50'000, 1600, 0.002);
UNACTIVE_TUNEABLE(MAX_NONPAWN_CORRHIST_BONUS, int, 7757, 0, 50'000, 1600, 0.002);

template <std::size_t size>
class HistoryBase {
    public:
    HistoryBase(const int& fill_value, const int& max_bonus)
        : fill_value(fill_value), max_bonus(max_bonus) {
        clear();
    }

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

    const int& fill_value;
    const int& max_bonus;
    std::array<int, size> history = {};
};

class ContinuationHistory: public HistoryBase<NUM_PIECES * NUM_SQUARES * NUM_PIECES * NUM_SQUARES> {
    public:
    ContinuationHistory(): HistoryBase(CONTHIST_FILL_VALUE, MAX_CONTHIST_BONUS) {}

    int& get(Piece prev_piece, Square prev_to, Piece piece, Square to);
};

class FromToHistory: public HistoryBase<NUM_COLORS * NUM_SQUARES * NUM_SQUARES> {
    public:
    FromToHistory(): HistoryBase(HIST_FILL_VALUE, MAX_HIST_BONUS) {}

    int& get(Color color, Square from, Square to);
};

class CaptureHistory: public HistoryBase<NUM_PIECES * NUM_SQUARES * NUM_PIECETYPES> {
    public:
    CaptureHistory(): HistoryBase(CAPTHIST_FILL_VALUE, MAX_CAPTHIST_BONUS) {}

    int& get(Piece piece, Square to, Piece captured);
};

class PawnCorrectionHistory: public HistoryBase<NUM_COLORS * PAWN_CORRHIST_SIZE> {
    public:
    PawnCorrectionHistory(): HistoryBase(PAWN_CORRHIST_FILL_VALUE, MAX_PAWN_CORRHIST_BONUS) {}

    int& get(Color color, uint16_t key);
};

class MinorCorrectionHistory: public HistoryBase<NUM_COLORS * MINOR_CORRHIST_SIZE> {
    public:
    MinorCorrectionHistory(): HistoryBase(MINOR_CORRHIST_FILL_VALUE, MAX_MINOR_CORRHIST_BONUS) {}

    int& get(Color color, uint16_t key);
};

class MajorCorrectionHistory: public HistoryBase<NUM_COLORS * MAJOR_CORRHIST_SIZE> {
    public:
    MajorCorrectionHistory(): HistoryBase(MAJOR_CORRHIST_FILL_VALUE, MAX_MAJOR_CORRHIST_BONUS) {}

    int& get(Color color, uint16_t key);
};

class NonPawnCorrectionHistory: public HistoryBase<NUM_COLORS * NONPAWN_CORRHIST_SIZE> {
    public:
    NonPawnCorrectionHistory(): HistoryBase(NONPAWN_CORRHIST_FILL_VALUE, MAX_NONPAWN_CORRHIST_BONUS) {}

    int& get(Color color, uint16_t key);
};
#pragma once

#include "chess.hpp"
#include <vector>

enum class TFlag: std::uint8_t {
    NO_FLAG,
    EXACT,
    LOWER_BOUND,
    UPPER_BOUND,
};

// side to move is not stored in the transposition table as it is in the zobrist hash
struct TEntry {
    uint64_t zobrist_hash = 0; // 8 bytes
    chess::Move best_move = chess::Move::NO_MOVE; // 8 bytes
    float evaluation = 0; // 4 bytes
    uint8_t depth = 0; // 1 byte
    TFlag flag = TFlag::NO_FLAG; // 1 byte
    uint8_t move_number = 0;
    // 1 padding byte
    // ==============
    // ----> total = 8 + 8 + 8 = 24 bytes
};

class TranspositionTable {
    public:
    TranspositionTable();
    void info();

    void allocateMB(int new_size);

    void allocateKB(int new_size);

    void store(uint64_t zobrist, float eval, int depth, chess::Move best, TFlag flag, uint8_t move_number);

    TEntry* probe(bool& is_hit, uint64_t zobrist);

    void clear();

    int hashfull();

    private:
    std::vector<TEntry> entries;
    int size_mb;
    int max_size_mb = 256;
};
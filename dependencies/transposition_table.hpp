#pragma once

#include "chess.hpp"
#include <vector>
#include <fstream>

enum class TFlag: uint8_t {
    NO_FLAG,
    EXACT,
    LOWER_BOUND,
    UPPER_BOUND,
};

// side to move is not stored in the transposition table as it is in the zobrist hash
struct TEntry {
    uint64_t zobrist_hash = 0; // 8 bytes
    float evaluation = 0; // 4 bytes
    uint16_t best_move = 0; // 2 bytes
    uint8_t depth_tflag = 0; // 1 byte  -> contains depth: 6 bits (max 63), flag: 2 bits (4 values)
    uint8_t move_number = 0; // 1 byte
    // ==============
    // ----> total = 8 + 8 = 16 bytes

    int depth(){
        return static_cast<int>(depth_tflag >> 2);
    };

    TFlag flag(){
        return static_cast<TFlag>(depth_tflag & 0b00000011);
    };
};

class TranspositionTable {
    public:
    TranspositionTable();
    void info();

    void allocateMB(int new_size);

    void store(uint64_t zobrist, float eval, int depth, chess::Move best, TFlag flag, uint8_t move_number);

    TEntry* probe(bool& is_hit, uint64_t zobrist);

    void clear();

    int hashfull();

    void save_to_file(std::string file);
    void load_from_file(std::string file);

    private:
    std::vector<TEntry> entries;
    int size_mb;
    int max_size_mb = 1024;
};


// side to move is not stored in the transposition table as it is in the zobrist hash
struct EvalEntry {
    uint32_t zobrist_hash = 0; // 8 bytes  -> hash has been cropped to 32 bits
    float evaluation = 0; // 4 bytes
    // ==============
    // ----> total = 4 + 4 = 8 bytes
};

class EvalTable {
    public:
    EvalTable();

    void allocateMB(int new_size);

    void store(uint64_t zobrist, float eval);

    float probe(bool& is_hit, uint64_t zobrist);

    private:
    std::vector<EvalEntry> entries;
};
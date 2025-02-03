#pragma once

#include "chess.hpp"
#include "misc.hpp"
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
    int16_t value_ = NO_VALUE; // 2 bytes
    int16_t eval_ = NO_VALUE; // 2 bytes
    uint16_t best_move = 0; // 2 bytes
    uint8_t depth_tflag = 0; // 1 byte  -> contains depth: 6 bits (64 values), flag: 2 bits (4 values)
    uint8_t move_number = 0; // 1 byte
    // ==============
    // ----> total = 8 + 8 = 16 bytes

    int depth(){
        return static_cast<int>(depth_tflag >> 2);
    };

    TFlag flag(){
        return static_cast<TFlag>(depth_tflag & 0b00000011);
    };

    int value(){
        return static_cast<int>(value_);
    };

    int eval(){
        return static_cast<int>(eval_);
    };

    TEntry(){};

    TEntry(uint64_t zobrist, int value, int eval, int depth, chess::Move move, TFlag flag, uint8_t move_number):
            zobrist_hash(zobrist),
            value_(value),
            eval_(eval),
            best_move(move.move()),
            depth_tflag((static_cast<uint8_t>(depth) << 2) | (static_cast<uint8_t>(flag))),
            move_number(move_number) {};
};

class TranspositionTable {
    public:
    TranspositionTable();
    void info();

    void allocateMB(int new_size);

    void store(uint64_t zobrist, int value, int eval, int depth, chess::Move move, TFlag flag, uint8_t move_number);
    void store(TEntry entry);

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
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
    uint64_t zobrist_hash   = 0; // 8 bytes
    int16_t value           = NO_VALUE; // 2 bytes
    int16_t static_eval     = NO_VALUE; // 2 bytes
    uint16_t move           = 0; // 2 bytes
    uint8_t depth_tflag     = 0; // 1 byte  -> contains depth: 6 bits (64 values), flag: 2 bits (4 values)
    uint8_t age     = 0; // 1 byte
    // ==============
    // ----> total = 8 + 8 = 16 bytes

    int depth(){
        return static_cast<int>(depth_tflag >> 2);
    };

    TFlag flag(){
        return static_cast<TFlag>(depth_tflag & 0b00000011);
    };

    TEntry(){};

    TEntry(uint64_t zobrist, int value, int static_eval, int depth, Move move, TFlag flag, uint8_t age):
            zobrist_hash(zobrist),
            value(value),
            static_eval(static_eval),
            move(move.move()),
            depth_tflag((static_cast<uint8_t>(depth) << 2) | (static_cast<uint8_t>(flag))),
            age(age) {};
};

struct TTData {
    int value               = NO_VALUE;
    int static_eval         = NO_VALUE;
    Move move               = Move::NO_MOVE;
    int depth               = 0;
    TFlag flag              = TFlag::NO_FLAG;
    int age                 = 0;

    TTData(){};

    TTData(uint64_t zobrist, int value, int static_eval, int depth, Move move, TFlag flag, uint8_t age, uint8_t move_number):
            value(value),
            static_eval(static_eval),
            move(move),
            depth(depth),
            flag(flag),
            age(age) {};

    TTData(TEntry* entry):
            value(entry->value),
            static_eval(entry->static_eval),
            move(entry->move),
            depth(entry->depth()),
            flag(entry->flag()),
            age(entry->age) {};
};

class TranspositionTable {
    public:
    TranspositionTable();
    void info();

    void allocateMB(int new_size);

    void increase_age();

    void store(uint64_t zobrist, int value, int eval, int depth, Move move, TFlag flag, bool pv);

    TTData probe(bool& is_hit, uint64_t zobrist);

    void clear();

    int hashfull();

    void save_to_stream(std::ofstream& ofs);
    void load_from_stream(std::ifstream& ifs);

    std::vector<TEntry> entries;
    private:
    int size_mb;
    int age;
};
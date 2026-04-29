#pragma once

#include "chess.hpp"
#include "misc.hpp"
#include <fstream>
#include <mutex>


enum class TFlag: uint8_t {
    NO_FLAG,
    EXACT,
    LOWER_BOUND,
    UPPER_BOUND,
};

// side to move is not stored in the transposition table as it is in the zobrist hash
struct TEntry {
    uint64_t zobrist_hash      = 0; // 8 bytes
    int16_t value              = NO_VALUE; // 2 bytes
    int16_t static_eval        = NO_VALUE; // 2 bytes
    uint16_t move              = 0; // 2 bytes
    uint8_t depth              = 0; // 1 byte
    uint8_t move_num_tflag_ttpv  = 0; // 1 byte -> contains move_num: 5 bits (32 values), flag: 2 bits (4 values), pv: 1 bit (2 values)
    // ==============
    // ----> total = 8 + 8 = 16 bytes

    int move_number(){
        return static_cast<int>(move_num_tflag_ttpv >> 3);
    };

    TFlag flag(){
        return static_cast<TFlag>(move_num_tflag_ttpv & 0b00000110);
    };

    bool ttpv(){
        return static_cast<bool>(move_num_tflag_ttpv & 0b00000001);
    };

    TEntry(){};
};

struct TTData {
    int value               = NO_VALUE;
    int static_eval         = NO_VALUE;
    Move move               = Move::NO_MOVE;
    int depth               = 0;
    int move_number         = 0;
    TFlag flag              = TFlag::NO_FLAG;
    bool ttpv               = false;

    TTData(){};

    TTData(TEntry* entry, bool pv):
            value(entry->value),
            static_eval(entry->static_eval),
            move(entry->move),
            depth(entry->depth),
            flag(entry->flag()),
            move_number(entry->move_number()),
            ttpv(entry->ttpv() || pv)
            {};
};

class TranspositionTable {
    public:
    TranspositionTable();
    ~TranspositionTable();
    void info();

    void allocateMB(int new_size);

    void store(uint64_t zobrist, int value, int eval, int depth, Move move, TFlag flag, int move_number, bool ttpv);

    TTData probe(bool& is_hit, uint64_t zobrist, bool pv);

    void clear();

    int hashfull();

    void save_to_stream(std::ofstream& ofs);
    void load_from_stream(std::ifstream& ifs);

    TEntry* entries = nullptr;
    size_t size = 0;
    private:
    int size_mb = 0;
    std::mutex clear_mutex;
};
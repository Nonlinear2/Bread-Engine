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

constexpr int MAX_AGE = (1 << 5) - 1;

// side to move is not stored in the transposition table as it is in the zobrist hash
struct TEntry {
    uint16_t zobrist_hash      = 0; // 2 bytes
    int16_t value              = NO_VALUE; // 2 bytes
    int16_t static_eval        = NO_VALUE; // 2 bytes
    uint16_t move              = 0; // 2 bytes
    uint8_t depth              = 0; // 1 byte
    uint8_t age_tflag_ttpv  = 0; // 1 byte -> contains age: 5 bits (32 values), flag: 2 bits (4 values), pv: 1 bit (2 values)
    // ==============
    // ----> total = 2 + 2 + 2 + 2 + 1 + 1 = 10 bytes

    int age(){
        return static_cast<int>(age_tflag_ttpv >> 3);
    };

    TFlag flag(){
        return static_cast<TFlag>((age_tflag_ttpv >> 1) & 0b00000011);
    };

    bool ttpv(){
        return static_cast<bool>(age_tflag_ttpv & 0b00000001);
    };

    TEntry(){};

    void store(uint64_t zobrist, int value, int eval, int depth, Move move, TFlag flag, int age, bool ttpv);
};

struct alignas(32) Cluster {
    TEntry entries[TT_ENTRIES_PER_CLUSTER];
    int8_t _pad[2];
};

struct TTData {
    TEntry* entry_ptr       = nullptr;
    int value               = NO_VALUE;
    int static_eval         = NO_VALUE;
    Move move               = Move::NO_MOVE;
    int depth               = 0;
    TFlag flag              = TFlag::NO_FLAG;
    bool ttpv               = false;
    int age                 = 0;

    TTData(){};

    TTData(TEntry* entry, bool pv):
            value(entry->value),
            static_eval(entry->static_eval),
            move(entry->move),
            depth(entry->depth),
            flag(entry->flag()),
            age(entry->age()),
            ttpv(entry->ttpv() || pv)
            {};
};

class TranspositionTable {
    public:
    TranspositionTable();
    ~TranspositionTable();
    void info();

    void allocateMB(int new_size);

    Cluster* index(uint64_t hash);

    void increase_age();

    TTData probe(bool& is_hit, TEntry*& new_entry, uint64_t zobrist, bool pv);

    void clear();

    int hashfull();

    void save_to_stream(std::ofstream& ofs);
    void load_from_stream(std::ifstream& ifs);

    Cluster* clusters = nullptr;
    size_t num_clusters = 0;
    size_t num_entries = 0;
    int age;
    private:
    int size_mb = 0;
    std::mutex clear_mutex;
};
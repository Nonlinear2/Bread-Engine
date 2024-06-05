#include "transposition_table.hpp"

TranspositionTable::TranspositionTable(){allocateMB(256);};

void TranspositionTable::info(){
    int used = 0;
    for (int i = 0; i < entries.size(); i++){
        used += (entries[i].zobrist_hash != 0);
    }
    int used_percentage = used*100/entries.size();

    std::cout << "====================" << std::endl;
    std::cout << "transposition table:" << std::endl;
    std::cout << "size " << size_mb << " MB" << std::endl;
    std::cout << "number of entries " << entries.size() << std::endl;
    std::cout << "used entries " << used << std::endl;
    std::cout << "used percentage " << used_percentage << "%" << std::endl;
    std::cout << "====================" << std::endl;
}

void TranspositionTable::allocateMB(int new_size){
    new_size = std::max(new_size, 0);
    new_size = std::min(new_size, max_size_mb);

    size_mb = new_size;

    int num_entries = (size_mb * 1e6)/sizeof(TEntry);
    entries.resize(num_entries, TEntry());
}

void TranspositionTable::allocateKB(int new_size){
    new_size = std::max(new_size, 0);
    new_size = std::min(new_size, max_size_mb*1'000);

    size_mb = new_size/1000;

    int num_entries = (new_size * 1e3)/sizeof(TEntry);
    entries.resize(num_entries, TEntry());
}


void TranspositionTable::store(uint64_t zobrist, float eval, int depth, chess::Move best, TFlag flag, uint8_t move_number){
    // no need to store the side to move, as it is in the zobrist hash.
    TEntry* entry = &entries[zobrist % entries.size()];
    if (entry->zobrist_hash != zobrist || flag == TFlag::EXACT || depth > entry->depth || move_number > entry->move_number){
        entry->best_move = best;
        entry->depth = static_cast<uint8_t>(depth);
        entry->evaluation = eval;
        entry->flag = flag;
        entry->zobrist_hash = zobrist;
        entry->move_number = move_number;
    };
}

TEntry* TranspositionTable::probe(bool& is_hit, uint64_t zobrist){
    TEntry* entry = &entries[zobrist % entries.size()];
    is_hit = (entry->zobrist_hash == zobrist);
    return entry;
}


void TranspositionTable::clear(){
    std::fill(entries.begin(), entries.end(), TEntry());
}


int TranspositionTable::hashfull(){
    int used = 0;
    for (int i = 0; i < 1000; i++){
        used += (entries[i].zobrist_hash != 0);
    }
    return used;
}
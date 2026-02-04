#include "transposition_table.hpp"

TranspositionTable::TranspositionTable(): age(0) {allocateMB(256);};

void TranspositionTable::info(){
    int used = 0;
    int num_move = 0;
    int num_depth_zero = 0;
    int num_exact = 0;
    int num_upper = 0;
    int num_lower = 0;

    for (int i = 0; i < entries.size(); i++){
        TEntry entry = entries[i];
        if (entry.zobrist_hash != 0){
            used++;

            num_move += (entry.move != Move::NO_MOVE);

            num_depth_zero += (entry.depth() == 0);

            switch (entry.flag()){
                case TFlag::EXACT:
                    num_exact++;
                    break;
                case TFlag::UPPER_BOUND:
                    num_upper++;
                    break;
                case TFlag::LOWER_BOUND:
                    num_lower++;
                    break;
                default:
                    break;
            }
        }
    }
    int used_percentage = used*100/entries.size();

    std::cout << "=================================" << std::endl;
    std::cout << "transposition table:" << std::endl;
    std::cout << "size " << size_mb << " MB" << std::endl;
    std::cout << "number of entries " << entries.size() << std::endl;
    std::cout << "used entries " << used << std::endl;
    std::cout << "used percentage " << used_percentage << "%" << std::endl;
    if (used != 0){
        std::cout << "following percentages are relative to used entries." << std::endl;
        std::cout << "depth zero percentage " << (num_depth_zero*100)/used << "%" << std::endl;
        std::cout << "has move percentage " << (num_move*100)/used << "%" << std::endl;
        std::cout << "exact eval percentage " << (num_exact*100)/used << "%" << std::endl;
        std::cout << "lower bound eval percentage " << (num_lower*100)/used << "%" << std::endl;
        std::cout << "upper bound eval percentage " << (num_upper*100)/used << "%" << std::endl;
    }
    std::cout << "=================================" << std::endl;
}

void TranspositionTable::allocateMB(int new_size){
    assert(new_size >= 2);
    assert((new_size & (new_size - 1)) == 0); // make sure the size is a power of 2

    new_size = std::max(new_size, TT_MIN_SIZE);
    new_size = std::min(new_size, TT_MAX_SIZE);

    size_mb = new_size;

    // closest power of 2 to 1'000'000 / 16 is 2^16 = 65536
    assert(sizeof(TEntry) == 16);
    constexpr int entries_in_one_mb = 65536;
    int num_entries = size_mb * entries_in_one_mb;

    entries.resize(num_entries, TEntry());
    entries.shrink_to_fit();
}

void TranspositionTable::increase_age(){
    age++;
    age %= 256;
}

void TranspositionTable::store(uint64_t zobrist, int value, int static_eval, int depth,
                               Move move, TFlag flag, bool pv){

    assert(move != Move::NULL_MOVE);

    // no need to store the side to move, as it is in the zobrist hash.
    TEntry* entry = &entries[zobrist & (entries.size() - 1)];

    // we replace the old entry if:
    // - the old entry is empty
    // - the old entry is more than 4 moves older than the recent entry
    // - the new depth is greater than the old depth
    // - the new depth is nonzero and an exact entry
    if (flag == TFlag::EXACT ||
        entry->zobrist_hash != zobrist ||
        depth > entry->depth() - 1 - 2*pv ||
        age != entry->age
    )
    {
        // add move if the old entry didn't hold the same position or if the new move is better
        if (entry->zobrist_hash != zobrist || move != Move::NO_MOVE)
            entry->move = move.move();

        entry->zobrist_hash = zobrist;
        entry->value = value;
        entry->static_eval = static_eval;
        entry->depth_tflag = (static_cast<uint8_t>(depth) << 2) | (static_cast<uint8_t>(flag));
        entry->age = age;
    };
}

TTData TranspositionTable::probe(bool& is_hit, uint64_t zobrist){
    assert((entries.size() & (entries.size() - 1)) == 0);
    TEntry* entry = &entries[zobrist & (entries.size() - 1)];
    is_hit = (entry->zobrist_hash == zobrist);

    if (is_hit)
        return TTData(entry);
    else
        return TTData();
}

void TranspositionTable::clear(){
    std::fill(entries.begin(), entries.end(), TEntry());
    age = 0;
}

int TranspositionTable::hashfull(){
    int used = 0;
    for (int i = 0; i < 1000; i++){
        used += (entries[i].zobrist_hash != 0);
    }
    return used;
}

void TranspositionTable::save_to_stream(std::ofstream& ofs){
    for (const auto& entry : entries) {
        ofs.write(reinterpret_cast<const char*>(&entry), sizeof(TEntry));
    }
}

void TranspositionTable::load_from_stream(std::ifstream& ifs){
    for (auto& entry : entries) {
        ifs.read(reinterpret_cast<char*>(&entry), sizeof(TEntry));
    }
}
#include "transposition_table.hpp"

TranspositionTable::TranspositionTable(){
    allocateMB(256);
};

TranspositionTable::~TranspositionTable(){
    delete[] clusters;
};

void TranspositionTable::info(){
    int used = 0;
    int num_move = 0;
    int num_depth_zero = 0;
    int num_exact = 0;
    int num_upper = 0;
    int num_lower = 0;

    for (int i = 0; i < num_clusters; i++){
        Cluster cluster = clusters[i];
        for (TEntry& entry: cluster.entries){
            if (entry.zobrist_hash != 0){
                used++;

                num_move += (entry.move != Move::NO_MOVE);

                num_depth_zero += (entry.depth == 0);

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
    }
    int used_percentage = used*100/num_entries;

    std::cout << "=================================" << std::endl;
    std::cout << "transposition table:" << std::endl;
    std::cout << "size " << size_mb << " MB" << std::endl;
    std::cout << "number of entries " << num_entries << std::endl;
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

void TranspositionTable::allocateMB(int new_size_mb){
    assert(new_size_mb >= 2);
    assert((new_size_mb & (new_size_mb - 1)) == 0); // make sure the size is a power of 2

    new_size_mb = std::max(new_size_mb, TT_MIN_SIZE);
    new_size_mb = std::min(new_size_mb, TT_MAX_SIZE);

    // closest power of 2 to 1'000'000 / 32 is 2^15 = 32768
    assert(sizeof(Cluster) == 32);
    constexpr int clusters_per_mb = 32768;
    num_clusters = new_size_mb * clusters_per_mb;
    num_entries = 3 * num_clusters;
    size_mb = new_size_mb;

    delete[] clusters;
    clusters = new Cluster[num_clusters];
}

Cluster* TranspositionTable::index(uint64_t hash){
    return &clusters[
        static_cast<std::uint64_t>((static_cast<unsigned __int128>(hash) * static_cast<unsigned __int128>(num_clusters)) >> 64)
    ];
}

TTData TranspositionTable::probe(bool& is_hit, TEntry*& new_entry, uint64_t zobrist, bool pv){
    assert((num_clusters & (num_clusters - 1)) == 0);

    is_hit = false;

    int worst_value = ENGINE_MAX_DEPTH; // 32 is max tt move number 

    Cluster* cluster = index(zobrist);
    for (TEntry& candidate: cluster->entries){
        if (candidate.zobrist_hash == (uint16_t)zobrist){
            new_entry = &candidate;
            is_hit = true;
            break;
        }
        int candidate_value = candidate.depth - candidate.move_number() / 6;
        if (candidate_value < worst_value){
            worst_value = candidate_value;
            new_entry = &candidate;
        }
    }
    assert(new_entry != nullptr);

    if (is_hit)
        return TTData(new_entry, pv);
    else
        return TTData();
}

void TranspositionTable::clear(){
    std::lock_guard<std::mutex> lock(clear_mutex);
    for (size_t i = 0; i < num_clusters; i++) {
        for (auto& entry: clusters[i].entries)
            entry = TEntry();
    }
}

int TranspositionTable::hashfull(){
    int used = 0;
    for (int i = 0; i < 1000; i++){
        for (auto& entry: clusters[i].entries)
            used += (entry.zobrist_hash != 0);
    }
    return used / TT_ENTRIES_PER_CLUSTER;
}

void TranspositionTable::save_to_stream(std::ofstream& ofs){
    ofs.write(reinterpret_cast<const char*>(clusters), num_clusters * sizeof(Cluster));
}

void TranspositionTable::load_from_stream(std::ifstream& ifs){
    ifs.read(reinterpret_cast<char*>(clusters), num_clusters * sizeof(Cluster));
}

void TEntry::store(uint64_t zobrist, int value, int static_eval, int depth,
                               Move move, TFlag flag, int move_number, bool ttpv){

    assert(move != Move::NULL_MOVE);

    // no need to store the side to move, as it is in the zobrist hash.

    // we replace the old entry if:
    // - the old entry is empty
    // - the old entry is more than 4 moves older than the recent entry
    // - the new depth is greater than the old depth
    // - the new depth is nonzero and an exact entry
    if (move_number / 2 > this->move_number() + 2 ||
        depth > this->depth - 1 - 2*ttpv || // this will be true if the old entry is empty
        (depth != DEPTH_QSEARCH && flag == TFlag::EXACT))
    {
        // add move if the old entry didn't hold the same position or if the new move is better
        if (this->zobrist_hash != (uint16_t)zobrist || move != Move::NO_MOVE)
            this->move = move.move();

        this->zobrist_hash = (uint16_t)zobrist;
        this->value = value;
        this->static_eval = static_eval;
        this->depth = depth;
        this->move_num_tflag_ttpv = (static_cast<uint8_t>(move_number / 2) << 3) | (static_cast<uint8_t>(flag) << 1) | ttpv;
    };
}
#pragma once 

#include <stack>

#include "chess.hpp"
#include "tbprobe.hpp"
#include "piece_square_tables.hpp"
#include "transposition_table.hpp"
#include "nnue.hpp"
#include "misc.hpp"
#include "constants.hpp"

using BothModifiedFeatures = std::array<ModifiedFeatures, 2>;

class NnueBoard;

class AllBitboards {
    public:
    Bitboard bb[2][6];

    AllBitboards();
    AllBitboards(const NnueBoard& pos);
};

class NnueBoard: public Board {
    public:

    NnueBoard();
    NnueBoard(std::string_view fen);
    
    ~NnueBoard();

    void synchronize();

    bool legal(Move move);

    void update_state(Move move, TranspositionTable& tt);

    void restore_state(Move move);

    int evaluate();

    bool is_stalemate();

    std::pair<std::vector<int>, std::vector<int>> get_features();
    std::vector<int> get_features(Color color);

    private:
    class AccumulatorsStack {
        public:
        AccumulatorsStack();
        Accumulators& push_empty();
        Accumulators& top();
        BothModifiedFeatures& top_update();
        void clear_top_update();
        void clear_top_update(Color color);
        void pop();
        void apply_lazy_updates();

        private:
        std::vector<Accumulators> stack = std::vector<Accumulators>(MAX_PLY + 1);
        std::vector<BothModifiedFeatures> queued_updates = std::vector<BothModifiedFeatures>(MAX_PLY + 1);
        int idx;
        
        friend class NnueBoard;
    };

    AccumulatorsStack accumulators_stack;

    // accessed by [bucket][stm][mirrored]
    std::array<std::array<std::array<std::pair<AllBitboards, Accumulator>, 2>, 2>, INPUT_BUCKET_COUNT> finny_table;

    void compute_top_update(Move move, Color color);
    std::pair<std::vector<int>, std::vector<int>> get_modified_features(
        Color stm, int mirror, int bucket, AllBitboards prev_pos, AllBitboards new_pos);

    bool is_updatable_move(Move move);
};
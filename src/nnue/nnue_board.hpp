#pragma once 

#include <stack>

#include "chess.hpp"
#include "tbprobe.hpp"
#include "piece_square_tables.hpp"
#include "transposition_table.hpp"
#include "nnue.hpp"
#include "misc.hpp"

using BothModifiedFeatures = std::array<ModifiedFeatures, 2>;

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

    private:
    class AccumulatorsStack {
        public:
        AccumulatorsStack();
        Accumulators& push_empty();
        Accumulators& top();
        void clear_top_update();
        void set_top_update(ModifiedFeatures modified_white, ModifiedFeatures modified_black);
        void pop();
        void apply_lazy_updates();

        private:
        std::vector<Accumulators> stack = std::vector<Accumulators>(MAX_PLY + 1);
        std::vector<BothModifiedFeatures> queued_updates = std::vector<BothModifiedFeatures>(MAX_PLY + 1);
        int idx;
    };

    AccumulatorsStack accumulators_stack;

    std::vector<Accumulators> finny_table = std::vector<Accumulators>(32);

    ModifiedFeatures get_modified_features(Move move, Color color);

    bool is_updatable_move(Move move);
};
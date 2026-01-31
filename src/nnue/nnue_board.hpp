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
    std::vector<int> get_features(Color color);

    private:
    class AccumulatorsStack {
        public:
        AccumulatorsStack();
        Accumulators& push_empty();
        Accumulators& top();
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

    void compute_top_update(Move move, Color color);

    bool is_updatable_move(Move move);
};
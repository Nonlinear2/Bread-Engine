#include "bread_engine_core.hpp"
#include <iostream>

int main(){
    Engine engine = Engine();

    std::vector<std::string> fens = {
        // "r2qkbnr/1pp2ppp/p1p5/4p3/4P1b1/5N1P/PPPP1PP1/RNBQ1RK1 b kq - 0 6",
        // "8/p4k2/1p1b3P/3P4/3P1pb1/4n3/P4RP1/2r2BKR b - - 2 36",
        "r2qbb1n/pppn2pk/3p4/4p3/2B1P1Q1/5R2/PPPP1PPP/1NB1K1NR w K - 0 4",
        "r2qbb1n/pppn2pk/3p4/4p3/2B1P1Q1/7R/PPPP1PPP/1NB1K1NR b K - 1 4",
        "r3bb1n/pppn2pk/3p4/4p3/2B1P1Qq/7R/PPPP1PPP/1NB1K1NR w K - 2 5",
        "r3bb1n/pppn2pk/3p4/4p3/2B1P1QR/8/PPPP1PPP/1NB1K1NR b K - 0 5",
        "r4b1n/pppn2pk/3p4/4p2b/2B1P1QR/8/PPPP1PPP/1NB1K1NR w K - 1 6",
        
    };
    // engine.transpositsion_table.info();

    for (int i = 0; i < fens.size(); i++){
        chess::Move best;
        std::cout << fens[i] << "\n";
        best = engine.search(fens[i], SearchLimit(LimitType::Depth, 4));
        // best = engine.search(fens[i], SearchLimit(LimitType::Nodes, 80327));
        std::cout << "best move " << best << "\n";
        std::cout << "eval " << best.score() << std::endl;
        std::cout << engine.current_depth << std::endl;
        // engine.transposition_table.info();
    }
    return 0;
}
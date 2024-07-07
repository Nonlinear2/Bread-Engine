#include "bread_engine_core.hpp"
#include <iostream>

int main(){
    Engine engine = Engine();
    std::vector<std::string> fens = {
        //"r2qkbnr/1pp2ppp/p1p5/4p3/4P1b1/5N1P/PPPP1PP1/RNBQ1RK1 b kq - 0 6",
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
        "rnbqkbnr/pp1ppppp/8/2p5/4P3/8/PPPP1PPP/RNBQKBNR w KQkq - 0 2",
        "rnbqkbnr/pp1p1ppp/4p3/2p5/4P3/5N2/PPPP1PPP/RNBQKB1R w KQkq - 0 3",
        "rnbqkbnr/1p1p1ppp/p3p3/2p5/4P3/2N2N2/PPPP1PPP/R1BQKB1R w KQkq - 0 4",
    };
    // engine.transposition_table.info();

    for (int i = 0; i < fens.size(); i++){
        chess::Move best;
        std::cout << fens[i] << "\n";
        best = engine.search(fens[i], 1089, 11, 11);
        std::cout << "best move " << best << "\n";
        std::cout << "eval " << best.score() << std::endl;
        std::cout << engine.current_depth << std::endl;
        // engine.transposition_table.info();
    }
    return 0;
}
#include "bread_engine_core.hpp"
#include <iostream>
int main(){
    Engine engine = Engine();
    std::vector<std::string> fens = {
        "r2qkbnr/1pp2ppp/p1p5/4p3/4P1b1/5N1P/PPPP1PP1/RNBQ1RK1 b kq - 0 6",
    };

    for (int i = 0; i < fens.size(); i++){
        chess::Move best;
        std::cout << fens[i] << "\n";
        best = engine.search(fens[i], 13878, 11, 11);
        std::cout << "best move " << best << "\n";
        std::cout << "eval " << best.score() << std::endl;
        std::cout << engine.search_depth << std::endl;
        engine.transposition_table.info();
    }
    return 0;
}
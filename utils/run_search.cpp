#include "bread_engine_core.hpp"
#include <iostream>

int main(){
    Engine engine = Engine();
    std::vector<std::string> fens = {
        "2k5/8/8/8/4P3/8/Kp6/3r4 b - - 1 75",
    };

    for (int i = 0; i < fens.size(); i++){
        chess::Move best;
        std::cout << fens[i] << "\n";
        best = engine.search(fens[i], 2878, 1, 30);
        std::cout << "best move " << best << "\n";
        std::cout << "eval " << best.score() << std::endl;
        std::cout << engine.search_depth << std::endl;
        engine.transposition_table.info();
    }
    return 0;
}
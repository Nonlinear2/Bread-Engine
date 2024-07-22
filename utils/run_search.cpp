#include "bread_engine_core.hpp"
#include <iostream>

int main(){
    Engine engine = Engine();
    // engine.inner_board.synchronize();
    // std::cout << engine.inner_board.evaluate() << std::endl;

    // engine.inner_board.setFen("1Q6/p3p1Qp/R2pk2n/1B4p1/1p3p2/1P1N1P1R/PBPPPK1P/8 w - - 0 1");
    // engine.inner_board.synchronize();
    // std::cout << engine.inner_board.evaluate() << std::endl;

    // engine.inner_board.setFen("4k3/8/8/2N1K3/1R6/2Q3B1/8/8 w - - 0 1");
    // engine.inner_board.synchronize();
    // std::cout << engine.inner_board.evaluate() << std::endl;

    std::vector<std::string> fens = {
        // "r2qkbnr/1pp2ppp/p1p5/4p3/4P1b1/5N1P/PPPP1PP1/RNBQ1RK1 b kq - 0 6",
        // "8/p4k2/1p1b3P/3P4/3P1pb1/4n3/P4RP1/2r2BKR b - - 2 36",
        "3r2k1/1p3pp1/4p2p/rb2P3/4B3/P7/1q3PPP/1R2R1K1 w - - 0 22",
        
    };
    // engine.transpositsion_table.info();

    for (int i = 0; i < fens.size(); i++){
        chess::Move best;
        std::cout << fens[i] << "\n";
        best = engine.search(fens[i], SearchLimit(LimitType::Time, 1100));
        // best = engine.search(fens[i], SearchLimit(LimitType::Nodes, 80327));
        // engine.transposition_table.info();
    }
    return 0;
}
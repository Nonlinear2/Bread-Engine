#include "bread_engine_core.hpp"
#include <iostream>

int main(){
    Engine engine = Engine();

    std::vector<std::string> fens = {
        // "r2qkbnr/1pp2ppp/p1p5/4p3/4P1b1/5N1P/PPPP1PP1/RNBQ1RK1 b kq - 0 6",
        // "8/p4k2/1p1b3P/3P4/3P1pb1/4n3/P4RP1/2r2BKR b - - 2 36",
        // "3r2k1/1p3pp1/4p2p/rb2P3/4B3/P7/1q3PPP/1R2R1K1 w - - 0 22",
        // chess::constants::STARTPOS,
        // "2r5/3k4/8/8/2P5/3K3p/8/8 b - - 0 1",
        // "2r5/8/3k4/8/2PK4/7p/8/8 b - - 2 2"
        "r1Q5/3K4/8/8/3k4/7p/8/8 b - - 0 10"
    };
    // engine.transpositsion_table.info();

    for (int i = 0; i < fens.size(); i++){
        std::cout << fens[i] << "\n";
        engine.search(fens[i], SearchLimit(LimitType::Depth, 11));
    }
    return 0;
}   
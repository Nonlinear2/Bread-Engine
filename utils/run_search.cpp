#include "core.hpp"
#include <iostream>

int main(){
    Engine engine = Engine();

    std::vector<std::string> fens = {
        // "r2qkbnr/1pp2ppp/p1p5/4p3/4P1b1/5N1P/PPPP1PP1/RNBQ1RK1 b kq - 0 6",
        // "8/p4k2/1p1b3P/3P4/3P1pb1/4n3/P4RP1/2r2BKR b - - 2 36",
        // "3r2k1/1p3pp1/4p2p/rb2P3/4B3/P7/1q3PPP/1R2R1K1 w - - 0 22",
        constants::STARTPOS,
        // "2r5/3k4/8/8/2P5/3K3p/8/8 b - - 0 1",
        // "2r5/8/3k4/8/2PK4/7p/8/8 b - - 2 2"
        // "r1Q5/3K4/8/8/3k4/7p/8/8 b - - 0 10"
        "rnbqkb1r/pppp1ppp/8/4p3/4Pn2/8/PPPP1PPP/RNB1KB1R w KQkq - 0 5",
        "rnb1kb1r/pppp1ppp/8/4p3/4P3/8/PPP1KPPq/1n6 w kq - 0 16"
    };
    // engine.transpositsion_table.info();

    // for (int i = 0; i < fens.size(); i++){
    //     NnueBoard cb;
    //     cb.setFen(fens[i]);
    //     cb.synchronize();

    //     chess::Move best_move = engine.search(cb.getFen(), SearchLimit(LimitType::Depth, 9));
    //     std::cout << "score: " << best_move.score() << std::endl;

    //     // std::cout << fens[i] << "\n";
    //     // std::cout << cb.evaluate() << std::endl;
    //     // engine.search(fens[i], SearchLimit(LimitType::Depth, 11));
    // }
    NnueBoard cb;
    cb.setFen(constants::STARTPOS);
    cb.synchronize();

    chess::Move best_move = engine.search(cb.getFen(), SearchLimit(LimitType::Time, 300));
    cb.setFen("rnbqkbnr/pppp1ppp/8/4p3/4P3/8/PPPP1PPP/RNBQKBNR w KQkq e6 0 2");
    best_move = engine.search(cb.getFen(), SearchLimit(LimitType::Time, 300));
    std::cout << "score: " << best_move.score() << std::endl;

    return 0;
}   
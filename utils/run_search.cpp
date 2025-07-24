#include "core.hpp"
#include "uci.hpp"
#include <iostream>

int main(){
    // Engine engine = Engine();

    // std::vector<std::string> fens = {
    //     // "r2qkbnr/1pp2ppp/p1p5/4p3/4P1b1/5N1P/PPPP1PP1/RNBQ1RK1 b kq - 0 6",
    //     // "8/p4k2/1p1b3P/3P4/3P1pb1/4n3/P4RP1/2r2BKR b - - 2 36",
    //     // "3r2k1/1p3pp1/4p2p/rb2P3/4B3/P7/1q3PPP/1R2R1K1 w - - 0 22",
    //     constants::STARTPOS,
    //     // "2r5/3k4/8/8/2P5/3K3p/8/8 b - - 0 1",
    //     // "2r5/8/3k4/8/2PK4/7p/8/8 b - - 2 2"
    //     // "r1Q5/3K4/8/8/3k4/7p/8/8 b - - 0 10"
    //     "rnbqkb1r/pppp1ppp/8/4p3/4Pn2/8/PPPP1PPP/RNB1KB1R w KQkq - 0 5",
    //     "rnb1kb1r/pppp1ppp/8/4p3/4P3/8/PPP1KPPq/1n6 w kq - 0 16"
    // };
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
    // NnueBoard cb;
    // cb.setFen(constants::STARTPOS);
    // cb.synchronize();

    // chess::Move best_move = engine.search(cb.getFen(), SearchLimit(LimitType::Nodes, 300));
    // cb.setFen("rnbqkbnr/pp1p1ppp/2p5/4p3/2B1P3/5Q2/PPPP1PPP/RNB1K1NR b KQkq - 1 3");
    // cb.synchronize();
    // best_move = engine.search(cb.getFen(), SearchLimit(LimitType::Nodes, 300));
    // std::cout << "score: " << best_move.score() << std::endl;

    UCIAgent uci_engine = UCIAgent();
    std::string input;
    bool running;

    // running = uci_engine.process_uci_command("setoption name hash value 8");
    // running = uci_engine.process_uci_command("setoption name SyzygyPath value C:/Users/nourb/OneDrive/Desktop/Chess_Engine/engine_executables/3-4-5_pieces_Syzygy/3-4-5");
    // std::this_thread::sleep_for(std::chrono::milliseconds(2500));

    uci_engine.engine.load_state("saved_state.bin");

    running = uci_engine.process_uci_command("position startpos moves d2d4 g7g6");

    // running = uci_engine.process_uci_command("position startpos moves d2d4 g7g6 c2c4 f8g7 b1c3 d7d6 g1f3 b8c6 e2e4 c8g4 c1e3 e7e5 d4d5 c6e7 f1e2 g7h6 f3g5 g4d7 e1g1 e7f5 g5f7 e8f7 e4f5 g6f5 f2f4 g8f6 c4c5 h8g8 g1h1 e5e4 d1b3 d6c5 e3c5 f7g7 b3b7 g7h8 d5d6 c7d6 c5d6 h6g7 d6e5 d8c8 b7c8 a8c8 f1d1 d7c6 e2a6 c8d8 a6c4 g8e8 d1d8 e8d8 a1d1 d8d1 c3d1 f6d5 c4d5 c6d5 e5g7 h8g7 b2b3 g7g6 h1g1 a7a6 g1f2 g6h5 g2g3 d5f7 f2e3 h5g4 d1f2 g4h5 h2h3 f7e6 f2d1 h5g6 d1c3 a6a5 e3d4 g6f6 c3d1 h7h6 d1e3 h6h5 h3h4 e6d7 d4c5 a5a4 b3b4 f6e6 a2a3 d7c8 b4b5 c8b7 b5b6 b7c8 c5c6 c8a6 b6b7 a6b7 c6b7 e6f6 b7b6 f6e6 b6c5 e6f6 c5d6 f6g6 e3d5 g6g7 d6e5 g7h8 e5f5 e4e3 d5e3 h8h7 f5e4 h7h8 e3d1 h8g7 d1b2 g7f7 b2a4 f7e6 f4f5 e6f6 a4c3 f6g7 e4f4 g7f7 c3d5 f7g8 d5e3 g8h8 e3d5 h8g7 f5f6 g7g8 d5e7 g8f7 e7d5 f7e6 f4e4 e6f7 e4f4 f7f8 f4e5 f8e8 e5e6 e8f8 f6f7 f8g7");
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    running = uci_engine.process_uci_command("go nodes 10000");
    std::this_thread::sleep_for(std::chrono::milliseconds(5500));
    std::cout << "ok\n";
    return 0;
}   
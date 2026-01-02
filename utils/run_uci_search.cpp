#include "core.hpp"
#include "uci.hpp"
#include <iostream>
#include <fstream>

int main(){
    UCIAgent uci_engine;
    std::ifstream file(bread_DEBUG_UCI_COMMANDS_PATH);
    std::string input;

    if (!file.is_open()) {
        std::cerr << "failed to open file" << std::endl;
        return 1;
    }

    // uci_engine.process_uci_command("setoption name Hash value 8");
    // uci_engine.process_uci_command("setoption name Nonsense value true");
    // uci_engine.process_uci_command("setoption name SyzygyPath value C:/Users/nourb/OneDrive/Desktop/Chess_Engine/engine_executables/3-4-5_pieces_Syzygy/3-4-5");
    // std::this_thread::sleep_for(std::chrono::milliseconds(2000));

    // uci_engine.engine.load_state("saved_state.bin");

    std::vector<bool> correct_nodes = {};
    while (std::getline(file, input)) {
        std::cout << "-> " << input << std::endl;

        bool running = uci_engine.process_uci_command(input);
        if (!running)
            break;

        auto tokens = UCIAgent::split_string(input);
        if (!tokens.empty() && tokens[0] == "go") {
            if (uci_engine.main_search_thread.joinable()) {
                uci_engine.main_search_thread.join();
            }
            correct_nodes.push_back(std::stoi(tokens[2]) == uci_engine.engine.nodes);
        }
    }

    if (uci_engine.main_search_thread.joinable()) {
        uci_engine.main_search_thread.join();
    }

    std::cout << "ok\n";
    for (int i = 0; i < correct_nodes.size(); i++){
        if (!correct_nodes[i])
        std::cout << "missmatch between go nodes and command number: " << i << std::endl;
    }
    return 0;
}

// -> go nodes 545801
// info depth 1 score cp 351 nodes 54 nps 54000 time 1 hashfull 901 pv b4b3
// info depth 2 score cp 351 nodes 108 nps 54000 time 2 hashfull 901 pv b4b3 a7b8
// info depth 3 score cp 351 nodes 172 nps 57333 time 3 hashfull 901 pv b4b3 a7b8 b3b2
// info depth 4 score cp 360 nodes 314 nps 78500 time 4 hashfull 901 pv b4b3 a7b8 b3b2 a1b1
// info depth 5 score cp 351 nodes 471 nps 78500 time 6 hashfull 901 pv b4b3 a7b8 b3b2 a1g1 a3a2
// info depth 6 score cp 466 nodes 1794 nps 224250 time 8 hashfull 901 pv b4b3 h3e6 g8e6 a7b8 a3a2
// info depth 7 score cp 452 nodes 2522 nps 252200 time 10 hashfull 901 pv b4b3 h3e6 g8e6 a7b8 a3a2 d6e4 b3b2
// info depth 8 score cp 528 nodes 5760 nps 384000 time 15 hashfull 901 pv b4b3 a1g1 b3b2 a7b8 a3a2 h3f5 g6f5
// info depth 9 score cp 611 nodes 10380 nps 494285 time 21 hashfull 901 pv b4b3 h3e6 g8e6 a7b8 b3b2 a1b1 e6a2 b1d1 b2b1q
// info depth 10 score cp 619 nodes 15695 nps 581296 time 27 hashfull 901 pv b4b3 h3e6 b3b2 a1b1 b8a8 e6g8 h7g8 a7c5 h5f4 d6e4      
// info depth 11 score cp 605 nodes 40019 nps 714625 time 56 hashfull 901 pv b4b3 d6c4 b3b2 c4b2 a3b2 a1b1 g8a2 a7b8 a2b1
// info depth 12 score cp 804 nodes 103686 nps 730183 time 142 hashfull 901 pv b4b3 a7b8 b3b2 a1f1 a3a2 d6e4 b2b1q e4f6 h5f6 b8e5 f6h5 h1g2
// info depth 13 score cp 834 nodes 169480 nps 873608 time 194 hashfull 901 pv b4b3 a7b8 b3b2 a1f1 a3a2 d6e4 b2b1q e4f6 h5f6 b8e5 f6h5 f1g1 g8c4
// info depth 14 score cp 830 nodes 180525 nps 876334 time 206 hashfull 901 pv b4b3 a7b8 b3b2 a1f1 a3a2 d6e4 b2b1q e4f6 h5f6 b8e5 g8e6 e5f6 e6h3 f1g1
// info depth 15 score cp 830 nodes 224731 nps 917269 time 245 hashfull 901 pv b4b3 a7b8 b3b2 a1f1 a3a2 d6e4 b2b1q e4f6 h5f6 b8e5 g8e6 e5f6 e6h3 f1g1 h3e6
// info depth 16 score cp 923 nodes 315801 nps 965752 time 327 hashfull 902 pv b4b3 a7b8 b3b2 a1g1 a3a2 d6e4 a2a1q e4f6 h5f6 b8e5 a1g1 h1g1 b2b1q g1f2 b1b6 e2e3
// info depth 17 score cp 954 nodes 476995 nps 961683 time 496 hashfull 902 pv b4b3 a7b8 b3b2 a1g1 a3a2 d6e4 a2a1q e4f6 h5f6 b8e5 a1g1 h1g1 b2b1q g1f2 f6h5 h3g4 b1b6
// info depth 17 score cp 954 nodes 545863 nps 961026 time 568 hashfull 902 pv b4b3 a7b8 b3b2 a1g1 a3a2 d6e4 a2a1q e4f6 h5f6 b8e5 a1g1 h1g1 b2b1q g1f2 b1b6 e2e3 f6h5 e5d4
// bestmove b4b3 ponder a7b8
// ok


// int main(){
//     UCIAgent uci_engine;
//     std::ifstream file(bread_DEBUG_UCI_COMMANDS_PATH);
//     std::string input;

//     if (!file.is_open()) {
//         std::cerr << "failed to open file" << std::endl;
//         return 1;
//     }
//     uci_engine.process_uci_command("setoption name Hash value 128");
//     uci_engine.process_uci_command("setoption name SyzygyPath value C:/Users/nourb/OneDrive/Desktop/Chess_Engine/engine_executables/3-4-5_pieces_Syzygy/3-4-5");
//     while (true){
//         uci_engine.process_uci_command("ucinewgame");
//         uci_engine.engine.load_state("saved_state.bin");
//         uci_engine.process_uci_command("position startpos moves g1f3 g8f6 g2g3 g7g6 b2b3 f8g7 c1b2 e8g8 f1g2 d7d6 e1g1 e7e5 d2d3 h7h6 c2c4 f8e8 b1c3 c7c6 c3e4 f6e4 d3e4 a7a5 d1d2 b8a6 a1d1 a6c5 f3e1 d8b6 e1c2 g7f8 c2e3 h6h5 g1h1 a5a4 b3b4 a4a3 b2c1 c5d7 d2c3 a8a4 c1d2 d7f6 f2f3 b6d4 c3b3 d4a7 b3c2 h5h4 g3h4 f6h5 e3g4 c8e6 g2h3 b7b5 c4b5 c6b5 d2e3 a7e7 e3g5 f7f6 g5d2 g8h7 f1g1 e7f7 d1a1 e8c8 c2d3 e6c4 d3c2 c8c7 g1c1 d6d5 e4d5 a4a7 d2e3 a7b7 d5d6 f8d6 c1g1 d6b4 g4h6 c4a2 c2d3 c7c3 d3c3 b4c3 h6f7 c3a1 f7d6 b7b8 g1a1 b5b4 e3c5 a2g8 c5a7");

//         input = "go wtime 363 btime 455 winc 500 binc 220";
//         // input = "go nodes " + std::to_string(((rand() << 15) ^ rand()) % 161'195 + 315801);
//         std::cout << "-> " << input << std::endl;

//         bool running = uci_engine.process_uci_command(input);
//         if (!running)
//             break;

//         auto tokens = UCIAgent::split_string(input);
//         if (!tokens.empty() && tokens[0] == "go") {
//             if (uci_engine.main_search_thread.joinable()) {
//                 uci_engine.main_search_thread.join();
//             }
//         }
//     }

//     if (uci_engine.main_search_thread.joinable()) {
//         uci_engine.main_search_thread.join();
//     }

//     // uci_engine.engine.save_state("saved_state.bin");

//     std::cout << "ok\n";
//     return 0;

//     // position startpos moves g1f3 g8f6 g2g3 g7g6 b2b3 f8g7 c1b2 e8g8 f1g2 d7d6 e1g1 e7e5 d2d3 h7h6 c2c4 f8e8 b1c3 c7c6 c3e4 f6e4 d3e4 a7a5 d1d2 b8a6 a1d1 a6c5 f3e1 d8b6 e1c2 g7f8 c2e3 h6h5 g1h1 a5a4 b3b4 a4a3 b2c1 c5d7 d2c3 a8a4 c1d2 d7f6 f2f3 b6d4 c3b3 d4a7 b3c2 h5h4 g3h4 f6h5 e3g4 c8e6 g2h3 b7b5 c4b5 c6b5 d2e3 a7e7 e3g5 f7f6 g5d2 g8h7 f1g1 e7f7 d1a1 e8c8 c2d3 e6c4 d3c2 c8c7 g1c1 d6d5 e4d5 a4a7 d2e3 a7b7 d5d6 f8d6 c1g1 d6b4 g4h6 c4a2 c2d3 c7c3 d3c3 b4c3 h6f7 c3a1 f7d6 b7b8 g1a1 b5b4 e3c5 a2g8 c5a7
//     // go nodes 545801
// }
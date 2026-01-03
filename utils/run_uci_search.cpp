#include "core.hpp"
#include "uci.hpp"
#include <iostream>
#include <fstream>

int main(){
    UCIAgent uci_engine;
    std::vector<std::string> commands = {
        "uci",
        "setoption name SyzygyPath value C:/Users/Nutzer/LCO/TB/3-4-5",
        "setoption name Hash value 128",
        "isready",
        "ucinewgame",
        "isready",
        "position startpos moves g1f3 g8f6 g2g3 g7g6 b2b3 f8g7 c1b2 e8g8 f1g2",
        "go nodes 2284940",
        "position startpos moves g1f3 g8f6 g2g3 g7g6 b2b3 f8g7 c1b2 e8g8 f1g2 d7d6 e1g1",
        "go nodes 2350399",
        "position startpos moves g1f3 g8f6 g2g3 g7g6 b2b3 f8g7 c1b2 e8g8 f1g2 d7d6 e1g1 e7e5 d2d3",
        "go nodes 2254476",
        "position startpos moves g1f3 g8f6 g2g3 g7g6 b2b3 f8g7 c1b2 e8g8 f1g2 d7d6 e1g1 e7e5 d2d3 h7h6 c2c4",
        "go nodes 2025034",
        "position startpos moves g1f3 g8f6 g2g3 g7g6 b2b3 f8g7 c1b2 e8g8 f1g2 d7d6 e1g1 e7e5 d2d3 h7h6 c2c4 f8e8 b1c3",
        "go nodes 1782875",
        "position startpos moves g1f3 g8f6 g2g3 g7g6 b2b3 f8g7 c1b2 e8g8 f1g2 d7d6 e1g1 e7e5 d2d3 h7h6 c2c4 f8e8 b1c3 c7c6 c3e4",
        "go nodes 1320574",
        "position startpos moves g1f3 g8f6 g2g3 g7g6 b2b3 f8g7 c1b2 e8g8 f1g2 d7d6 e1g1 e7e5 d2d3 h7h6 c2c4 f8e8 b1c3 c7c6 c3e4 f6e4 d3e4",
        "go nodes 1862131",
        "position startpos moves g1f3 g8f6 g2g3 g7g6 b2b3 f8g7 c1b2 e8g8 f1g2 d7d6 e1g1 e7e5 d2d3 h7h6 c2c4 f8e8 b1c3 c7c6 c3e4 f6e4 d3e4 a7a5 d1d2",
        "go nodes 1207331",
        "position startpos moves g1f3 g8f6 g2g3 g7g6 b2b3 f8g7 c1b2 e8g8 f1g2 d7d6 e1g1 e7e5 d2d3 h7h6 c2c4 f8e8 b1c3 c7c6 c3e4 f6e4 d3e4 a7a5 d1d2 b8a6 a1d1",
        "go nodes 1061451",
        "position startpos moves g1f3 g8f6 g2g3 g7g6 b2b3 f8g7 c1b2 e8g8 f1g2 d7d6 e1g1 e7e5 d2d3 h7h6 c2c4 f8e8 b1c3 c7c6 c3e4 f6e4 d3e4 a7a5 d1d2 b8a6 a1d1 a6c5 f3e1",
        "go nodes 1578067",
        "position startpos moves g1f3 g8f6 g2g3 g7g6 b2b3 f8g7 c1b2 e8g8 f1g2 d7d6 e1g1 e7e5 d2d3 h7h6 c2c4 f8e8 b1c3 c7c6 c3e4 f6e4 d3e4 a7a5 d1d2 b8a6 a1d1 a6c5 f3e1 d8b6 e1c2",
        "go nodes 1580972",
        "position startpos moves g1f3 g8f6 g2g3 g7g6 b2b3 f8g7 c1b2 e8g8 f1g2 d7d6 e1g1 e7e5 d2d3 h7h6 c2c4 f8e8 b1c3 c7c6 c3e4 f6e4 d3e4 a7a5 d1d2 b8a6 a1d1 a6c5 f3e1 d8b6 e1c2 g7f8 c2e3",
        "go nodes 1457561",
        "position startpos moves g1f3 g8f6 g2g3 g7g6 b2b3 f8g7 c1b2 e8g8 f1g2 d7d6 e1g1 e7e5 d2d3 h7h6 c2c4 f8e8 b1c3 c7c6 c3e4 f6e4 d3e4 a7a5 d1d2 b8a6 a1d1 a6c5 f3e1 d8b6 e1c2 g7f8 c2e3 h6h5 g1h1",
        "go nodes 1468935",
        "position startpos moves g1f3 g8f6 g2g3 g7g6 b2b3 f8g7 c1b2 e8g8 f1g2 d7d6 e1g1 e7e5 d2d3 h7h6 c2c4 f8e8 b1c3 c7c6 c3e4 f6e4 d3e4 a7a5 d1d2 b8a6 a1d1 a6c5 f3e1 d8b6 e1c2 g7f8 c2e3 h6h5 g1h1 a5a4 b3b4",
        "go nodes 1296854",
        "position startpos moves g1f3 g8f6 g2g3 g7g6 b2b3 f8g7 c1b2 e8g8 f1g2 d7d6 e1g1 e7e5 d2d3 h7h6 c2c4 f8e8 b1c3 c7c6 c3e4 f6e4 d3e4 a7a5 d1d2 b8a6 a1d1 a6c5 f3e1 d8b6 e1c2 g7f8 c2e3 h6h5 g1h1 a5a4 b3b4 a4a3 b2c1",
        "go nodes 1304711",
        "position startpos moves g1f3 g8f6 g2g3 g7g6 b2b3 f8g7 c1b2 e8g8 f1g2 d7d6 e1g1 e7e5 d2d3 h7h6 c2c4 f8e8 b1c3 c7c6 c3e4 f6e4 d3e4 a7a5 d1d2 b8a6 a1d1 a6c5 f3e1 d8b6 e1c2 g7f8 c2e3 h6h5 g1h1 a5a4 b3b4 a4a3 b2c1 c5d7 d2c3",
        "go nodes 1212341",
        "position startpos moves g1f3 g8f6 g2g3 g7g6 b2b3 f8g7 c1b2 e8g8 f1g2 d7d6 e1g1 e7e5 d2d3 h7h6 c2c4 f8e8 b1c3 c7c6 c3e4 f6e4 d3e4 a7a5 d1d2 b8a6 a1d1 a6c5 f3e1 d8b6 e1c2 g7f8 c2e3 h6h5 g1h1 a5a4 b3b4 a4a3 b2c1 c5d7 d2c3 a8a4 c1d2",
        "go nodes 1036099",
        "position startpos moves g1f3 g8f6 g2g3 g7g6 b2b3 f8g7 c1b2 e8g8 f1g2 d7d6 e1g1 e7e5 d2d3 h7h6 c2c4 f8e8 b1c3 c7c6 c3e4 f6e4 d3e4 a7a5 d1d2 b8a6 a1d1 a6c5 f3e1 d8b6 e1c2 g7f8 c2e3 h6h5 g1h1 a5a4 b3b4 a4a3 b2c1 c5d7 d2c3 a8a4 c1d2 d7f6 f2f3",
        "go nodes 1013372",
        "position startpos moves g1f3 g8f6 g2g3 g7g6 b2b3 f8g7 c1b2 e8g8 f1g2 d7d6 e1g1 e7e5 d2d3 h7h6 c2c4 f8e8 b1c3 c7c6 c3e4 f6e4 d3e4 a7a5 d1d2 b8a6 a1d1 a6c5 f3e1 d8b6 e1c2 g7f8 c2e3 h6h5 g1h1 a5a4 b3b4 a4a3 b2c1 c5d7 d2c3 a8a4 c1d2 d7f6 f2f3 b6d4 c3b3",
        "go nodes 1165070",
        "position startpos moves g1f3 g8f6 g2g3 g7g6 b2b3 f8g7 c1b2 e8g8 f1g2 d7d6 e1g1 e7e5 d2d3 h7h6 c2c4 f8e8 b1c3 c7c6 c3e4 f6e4 d3e4 a7a5 d1d2 b8a6 a1d1 a6c5 f3e1 d8b6 e1c2 g7f8 c2e3 h6h5 g1h1 a5a4 b3b4 a4a3 b2c1 c5d7 d2c3 a8a4 c1d2 d7f6 f2f3 b6d4 c3b3 d4a7 b3c2",
        "go nodes 1226961",
        "position startpos moves g1f3 g8f6 g2g3 g7g6 b2b3 f8g7 c1b2 e8g8 f1g2 d7d6 e1g1 e7e5 d2d3 h7h6 c2c4 f8e8 b1c3 c7c6 c3e4 f6e4 d3e4 a7a5 d1d2 b8a6 a1d1 a6c5 f3e1 d8b6 e1c2 g7f8 c2e3 h6h5 g1h1 a5a4 b3b4 a4a3 b2c1 c5d7 d2c3 a8a4 c1d2 d7f6 f2f3 b6d4 c3b3 d4a7 b3c2 h5h4 g3h4",
        "go nodes 1034108",
        "position startpos moves g1f3 g8f6 g2g3 g7g6 b2b3 f8g7 c1b2 e8g8 f1g2 d7d6 e1g1 e7e5 d2d3 h7h6 c2c4 f8e8 b1c3 c7c6 c3e4 f6e4 d3e4 a7a5 d1d2 b8a6 a1d1 a6c5 f3e1 d8b6 e1c2 g7f8 c2e3 h6h5 g1h1 a5a4 b3b4 a4a3 b2c1 c5d7 d2c3 a8a4 c1d2 d7f6 f2f3 b6d4 c3b3 d4a7 b3c2 h5h4 g3h4 f6h5 e3g4",
        "go nodes 828823",
        "position startpos moves g1f3 g8f6 g2g3 g7g6 b2b3 f8g7 c1b2 e8g8 f1g2 d7d6 e1g1 e7e5 d2d3 h7h6 c2c4 f8e8 b1c3 c7c6 c3e4 f6e4 d3e4 a7a5 d1d2 b8a6 a1d1 a6c5 f3e1 d8b6 e1c2 g7f8 c2e3 h6h5 g1h1 a5a4 b3b4 a4a3 b2c1 c5d7 d2c3 a8a4 c1d2 d7f6 f2f3 b6d4 c3b3 d4a7 b3c2 h5h4 g3h4 f6h5 e3g4 c8e6 g2h3",
        "go nodes 1210223",
        "position startpos moves g1f3 g8f6 g2g3 g7g6 b2b3 f8g7 c1b2 e8g8 f1g2 d7d6 e1g1 e7e5 d2d3 h7h6 c2c4 f8e8 b1c3 c7c6 c3e4 f6e4 d3e4 a7a5 d1d2 b8a6 a1d1 a6c5 f3e1 d8b6 e1c2 g7f8 c2e3 h6h5 g1h1 a5a4 b3b4 a4a3 b2c1 c5d7 d2c3 a8a4 c1d2 d7f6 f2f3 b6d4 c3b3 d4a7 b3c2 h5h4 g3h4 f6h5 e3g4 c8e6 g2h3 b7b5 c4b5",
        "go nodes 1152096",
        "position startpos moves g1f3 g8f6 g2g3 g7g6 b2b3 f8g7 c1b2 e8g8 f1g2 d7d6 e1g1 e7e5 d2d3 h7h6 c2c4 f8e8 b1c3 c7c6 c3e4 f6e4 d3e4 a7a5 d1d2 b8a6 a1d1 a6c5 f3e1 d8b6 e1c2 g7f8 c2e3 h6h5 g1h1 a5a4 b3b4 a4a3 b2c1 c5d7 d2c3 a8a4 c1d2 d7f6 f2f3 b6d4 c3b3 d4a7 b3c2 h5h4 g3h4 f6h5 e3g4 c8e6 g2h3 b7b5 c4b5 c6b5 d2e3",
        "go nodes 1223114",
        "position startpos moves g1f3 g8f6 g2g3 g7g6 b2b3 f8g7 c1b2 e8g8 f1g2 d7d6 e1g1 e7e5 d2d3 h7h6 c2c4 f8e8 b1c3 c7c6 c3e4 f6e4 d3e4 a7a5 d1d2 b8a6 a1d1 a6c5 f3e1 d8b6 e1c2 g7f8 c2e3 h6h5 g1h1 a5a4 b3b4 a4a3 b2c1 c5d7 d2c3 a8a4 c1d2 d7f6 f2f3 b6d4 c3b3 d4a7 b3c2 h5h4 g3h4 f6h5 e3g4 c8e6 g2h3 b7b5 c4b5 c6b5 d2e3 a7e7 e3g5",
        "go nodes 578080",
        "position startpos moves g1f3 g8f6 g2g3 g7g6 b2b3 f8g7 c1b2 e8g8 f1g2 d7d6 e1g1 e7e5 d2d3 h7h6 c2c4 f8e8 b1c3 c7c6 c3e4 f6e4 d3e4 a7a5 d1d2 b8a6 a1d1 a6c5 f3e1 d8b6 e1c2 g7f8 c2e3 h6h5 g1h1 a5a4 b3b4 a4a3 b2c1 c5d7 d2c3 a8a4 c1d2 d7f6 f2f3 b6d4 c3b3 d4a7 b3c2 h5h4 g3h4 f6h5 e3g4 c8e6 g2h3 b7b5 c4b5 c6b5 d2e3 a7e7 e3g5 f7f6 g5d2",
        "go nodes 525356",
        "position startpos moves g1f3 g8f6 g2g3 g7g6 b2b3 f8g7 c1b2 e8g8 f1g2 d7d6 e1g1 e7e5 d2d3 h7h6 c2c4 f8e8 b1c3 c7c6 c3e4 f6e4 d3e4 a7a5 d1d2 b8a6 a1d1 a6c5 f3e1 d8b6 e1c2 g7f8 c2e3 h6h5 g1h1 a5a4 b3b4 a4a3 b2c1 c5d7 d2c3 a8a4 c1d2 d7f6 f2f3 b6d4 c3b3 d4a7 b3c2 h5h4 g3h4 f6h5 e3g4 c8e6 g2h3 b7b5 c4b5 c6b5 d2e3 a7e7 e3g5 f7f6 g5d2 g8h7 f1g1",
        "go nodes 797828",
        "position startpos moves g1f3 g8f6 g2g3 g7g6 b2b3 f8g7 c1b2 e8g8 f1g2 d7d6 e1g1 e7e5 d2d3 h7h6 c2c4 f8e8 b1c3 c7c6 c3e4 f6e4 d3e4 a7a5 d1d2 b8a6 a1d1 a6c5 f3e1 d8b6 e1c2 g7f8 c2e3 h6h5 g1h1 a5a4 b3b4 a4a3 b2c1 c5d7 d2c3 a8a4 c1d2 d7f6 f2f3 b6d4 c3b3 d4a7 b3c2 h5h4 g3h4 f6h5 e3g4 c8e6 g2h3 b7b5 c4b5 c6b5 d2e3 a7e7 e3g5 f7f6 g5d2 g8h7 f1g1 e7f7 d1a1        ",
        "go nodes 446076",
        "position startpos moves g1f3 g8f6 g2g3 g7g6 b2b3 f8g7 c1b2 e8g8 f1g2 d7d6 e1g1 e7e5 d2d3 h7h6 c2c4 f8e8 b1c3 c7c6 c3e4 f6e4 d3e4 a7a5 d1d2 b8a6 a1d1 a6c5 f3e1 d8b6 e1c2 g7f8 c2e3 h6h5 g1h1 a5a4 b3b4 a4a3 b2c1 c5d7 d2c3 a8a4 c1d2 d7f6 f2f3 b6d4 c3b3 d4a7 b3c2 h5h4 g3h4 f6h5 e3g4 c8e6 g2h3 b7b5 c4b5 c6b5 d2e3 a7e7 e3g5 f7f6 g5d2 g8h7 f1g1 e7f7 d1a1 e8c8 c2d3",
        "go nodes 541796",
        "position startpos moves g1f3 g8f6 g2g3 g7g6 b2b3 f8g7 c1b2 e8g8 f1g2 d7d6 e1g1 e7e5 d2d3 h7h6 c2c4 f8e8 b1c3 c7c6 c3e4 f6e4 d3e4 a7a5 d1d2 b8a6 a1d1 a6c5 f3e1 d8b6 e1c2 g7f8 c2e3 h6h5 g1h1 a5a4 b3b4 a4a3 b2c1 c5d7 d2c3 a8a4 c1d2 d7f6 f2f3 b6d4 c3b3 d4a7 b3c2 h5h4 g3h4 f6h5 e3g4 c8e6 g2h3 b7b5 c4b5 c6b5 d2e3 a7e7 e3g5 f7f6 g5d2 g8h7 f1g1 e7f7 d1a1 e8c8 c2d3 e6c4 d3c2",
        "go nodes 664287",
        "position startpos moves g1f3 g8f6 g2g3 g7g6 b2b3 f8g7 c1b2 e8g8 f1g2 d7d6 e1g1 e7e5 d2d3 h7h6 c2c4 f8e8 b1c3 c7c6 c3e4 f6e4 d3e4 a7a5 d1d2 b8a6 a1d1 a6c5 f3e1 d8b6 e1c2 g7f8 c2e3 h6h5 g1h1 a5a4 b3b4 a4a3 b2c1 c5d7 d2c3 a8a4 c1d2 d7f6 f2f3 b6d4 c3b3 d4a7 b3c2 h5h4 g3h4 f6h5 e3g4 c8e6 g2h3 b7b5 c4b5 c6b5 d2e3 a7e7 e3g5 f7f6 g5d2 g8h7 f1g1 e7f7 d1a1 e8c8 c2d3 e6c4 d3c2 c8c7 g1c1",
        "go nodes 617917",
        "position startpos moves g1f3 g8f6 g2g3 g7g6 b2b3 f8g7 c1b2 e8g8 f1g2 d7d6 e1g1 e7e5 d2d3 h7h6 c2c4 f8e8 b1c3 c7c6 c3e4 f6e4 d3e4 a7a5 d1d2 b8a6 a1d1 a6c5 f3e1 d8b6 e1c2 g7f8 c2e3 h6h5 g1h1 a5a4 b3b4 a4a3 b2c1 c5d7 d2c3 a8a4 c1d2 d7f6 f2f3 b6d4 c3b3 d4a7 b3c2 h5h4 g3h4 f6h5 e3g4 c8e6 g2h3 b7b5 c4b5 c6b5 d2e3 a7e7 e3g5 f7f6 g5d2 g8h7 f1g1 e7f7 d1a1 e8c8 c2d3 e6c4 d3c2 c8c7 g1c1 d6d5 e4d5",
        "go nodes 538636",
        "position startpos moves g1f3 g8f6 g2g3 g7g6 b2b3 f8g7 c1b2 e8g8 f1g2 d7d6 e1g1 e7e5 d2d3 h7h6 c2c4 f8e8 b1c3 c7c6 c3e4 f6e4 d3e4 a7a5 d1d2 b8a6 a1d1 a6c5 f3e1 d8b6 e1c2 g7f8 c2e3 h6h5 g1h1 a5a4 b3b4 a4a3 b2c1 c5d7 d2c3 a8a4 c1d2 d7f6 f2f3 b6d4 c3b3 d4a7 b3c2 h5h4 g3h4 f6h5 e3g4 c8e6 g2h3 b7b5 c4b5 c6b5 d2e3 a7e7 e3g5 f7f6 g5d2 g8h7 f1g1 e7f7 d1a1 e8c8 c2d3 e6c4 d3c2 c8c7 g1c1 d6d5 e4d5 a4a7 d2e3",
        "go nodes 864759",
        "position startpos moves g1f3 g8f6 g2g3 g7g6 b2b3 f8g7 c1b2 e8g8 f1g2 d7d6 e1g1 e7e5 d2d3 h7h6 c2c4 f8e8 b1c3 c7c6 c3e4 f6e4 d3e4 a7a5 d1d2 b8a6 a1d1 a6c5 f3e1 d8b6 e1c2 g7f8 c2e3 h6h5 g1h1 a5a4 b3b4 a4a3 b2c1 c5d7 d2c3 a8a4 c1d2 d7f6 f2f3 b6d4 c3b3 d4a7 b3c2 h5h4 g3h4 f6h5 e3g4 c8e6 g2h3 b7b5 c4b5 c6b5 d2e3 a7e7 e3g5 f7f6 g5d2 g8h7 f1g1 e7f7 d1a1 e8c8 c2d3 e6c4 d3c2 c8c7 g1c1 d6d5 e4d5 a4a7 d2e3 a7b7 d5d6",
        "go nodes 1078314",
        "position startpos moves g1f3 g8f6 g2g3 g7g6 b2b3 f8g7 c1b2 e8g8 f1g2 d7d6 e1g1 e7e5 d2d3 h7h6 c2c4 f8e8 b1c3 c7c6 c3e4 f6e4 d3e4 a7a5 d1d2 b8a6 a1d1 a6c5 f3e1 d8b6 e1c2 g7f8 c2e3 h6h5 g1h1 a5a4 b3b4 a4a3 b2c1 c5d7 d2c3 a8a4 c1d2 d7f6 f2f3 b6d4 c3b3 d4a7 b3c2 h5h4 g3h4 f6h5 e3g4 c8e6 g2h3 b7b5 c4b5 c6b5 d2e3 a7e7 e3g5 f7f6 g5d2 g8h7 f1g1 e7f7 d1a1 e8c8 c2d3 e6c4 d3c2 c8c7 g1c1 d6d5 e4d5 a4a7 d2e3 a7b7 d5d6 f8d6 c1g1",
        "go nodes 1072385",
        "position startpos moves g1f3 g8f6 g2g3 g7g6 b2b3 f8g7 c1b2 e8g8 f1g2 d7d6 e1g1 e7e5 d2d3 h7h6 c2c4 f8e8 b1c3 c7c6 c3e4 f6e4 d3e4 a7a5 d1d2 b8a6 a1d1 a6c5 f3e1 d8b6 e1c2 g7f8 c2e3 h6h5 g1h1 a5a4 b3b4 a4a3 b2c1 c5d7 d2c3 a8a4 c1d2 d7f6 f2f3 b6d4 c3b3 d4a7 b3c2 h5h4 g3h4 f6h5 e3g4 c8e6 g2h3 b7b5 c4b5 c6b5 d2e3 a7e7 e3g5 f7f6 g5d2 g8h7 f1g1 e7f7 d1a1 e8c8 c2d3 e6c4 d3c2 c8c7 g1c1 d6d5 e4d5 a4a7 d2e3 a7b7 d5d6 f8d6 c1g1 d6b4 g4h6",
        "go nodes 1153366",
        "position startpos moves g1f3 g8f6 g2g3 g7g6 b2b3 f8g7 c1b2 e8g8 f1g2 d7d6 e1g1 e7e5 d2d3 h7h6 c2c4 f8e8 b1c3 c7c6 c3e4 f6e4 d3e4 a7a5 d1d2 b8a6 a1d1 a6c5 f3e1 d8b6 e1c2 g7f8 c2e3 h6h5 g1h1 a5a4 b3b4 a4a3 b2c1 c5d7 d2c3 a8a4 c1d2 d7f6 f2f3 b6d4 c3b3 d4a7 b3c2 h5h4 g3h4 f6h5 e3g4 c8e6 g2h3 b7b5 c4b5 c6b5 d2e3 a7e7 e3g5 f7f6 g5d2 g8h7 f1g1 e7f7 d1a1 e8c8 c2d3 e6c4 d3c2 c8c7 g1c1 d6d5 e4d5 a4a7 d2e3 a7b7 d5d6 f8d6 c1g1 d6b4 g4h6 c4a2 c2d3       ",
        "go nodes 1087620",
        "position startpos moves g1f3 g8f6 g2g3 g7g6 b2b3 f8g7 c1b2 e8g8 f1g2 d7d6 e1g1 e7e5 d2d3 h7h6 c2c4 f8e8 b1c3 c7c6 c3e4 f6e4 d3e4 a7a5 d1d2 b8a6 a1d1 a6c5 f3e1 d8b6 e1c2 g7f8 c2e3 h6h5 g1h1 a5a4 b3b4 a4a3 b2c1 c5d7 d2c3 a8a4 c1d2 d7f6 f2f3 b6d4 c3b3 d4a7 b3c2 h5h4 g3h4 f6h5 e3g4 c8e6 g2h3 b7b5 c4b5 c6b5 d2e3 a7e7 e3g5 f7f6 g5d2 g8h7 f1g1 e7f7 d1a1 e8c8 c2d3 e6c4 d3c2 c8c7 g1c1 d6d5 e4d5 a4a7 d2e3 a7b7 d5d6 f8d6 c1g1 d6b4 g4h6 c4a2 c2d3 c7c3 d3c3",
        "go nodes 1230519",
        "position startpos moves g1f3 g8f6 g2g3 g7g6 b2b3 f8g7 c1b2 e8g8 f1g2 d7d6 e1g1 e7e5 d2d3 h7h6 c2c4 f8e8 b1c3 c7c6 c3e4 f6e4 d3e4 a7a5 d1d2 b8a6 a1d1 a6c5 f3e1 d8b6 e1c2 g7f8 c2e3 h6h5 g1h1 a5a4 b3b4 a4a3 b2c1 c5d7 d2c3 a8a4 c1d2 d7f6 f2f3 b6d4 c3b3 d4a7 b3c2 h5h4 g3h4 f6h5 e3g4 c8e6 g2h3 b7b5 c4b5 c6b5 d2e3 a7e7 e3g5 f7f6 g5d2 g8h7 f1g1 e7f7 d1a1 e8c8 c2d3 e6c4 d3c2 c8c7 g1c1 d6d5 e4d5 a4a7 d2e3 a7b7 d5d6 f8d6 c1g1 d6b4 g4h6 c4a2 c2d3 c7c3 d3c3 b4c3 h6f7",
        "go nodes 1005751",
        "position startpos moves g1f3 g8f6 g2g3 g7g6 b2b3 f8g7 c1b2 e8g8 f1g2 d7d6 e1g1 e7e5 d2d3 h7h6 c2c4 f8e8 b1c3 c7c6 c3e4 f6e4 d3e4 a7a5 d1d2 b8a6 a1d1 a6c5 f3e1 d8b6 e1c2 g7f8 c2e3 h6h5 g1h1 a5a4 b3b4 a4a3 b2c1 c5d7 d2c3 a8a4 c1d2 d7f6 f2f3 b6d4 c3b3 d4a7 b3c2 h5h4 g3h4 f6h5 e3g4 c8e6 g2h3 b7b5 c4b5 c6b5 d2e3 a7e7 e3g5 f7f6 g5d2 g8h7 f1g1 e7f7 d1a1 e8c8 c2d3 e6c4 d3c2 c8c7 g1c1 d6d5 e4d5 a4a7 d2e3 a7b7 d5d6 f8d6 c1g1 d6b4 g4h6 c4a2 c2d3 c7c3 d3c3 b4c3 h6f7 c3a1 f7d6",
        "go nodes 701444",
        "position startpos moves g1f3 g8f6 g2g3 g7g6 b2b3 f8g7 c1b2 e8g8 f1g2 d7d6 e1g1 e7e5 d2d3 h7h6 c2c4 f8e8 b1c3 c7c6 c3e4 f6e4 d3e4 a7a5 d1d2 b8a6 a1d1 a6c5 f3e1 d8b6 e1c2 g7f8 c2e3 h6h5 g1h1 a5a4 b3b4 a4a3 b2c1 c5d7 d2c3 a8a4 c1d2 d7f6 f2f3 b6d4 c3b3 d4a7 b3c2 h5h4 g3h4 f6h5 e3g4 c8e6 g2h3 b7b5 c4b5 c6b5 d2e3 a7e7 e3g5 f7f6 g5d2 g8h7 f1g1 e7f7 d1a1 e8c8 c2d3 e6c4 d3c2 c8c7 g1c1 d6d5 e4d5 a4a7 d2e3 a7b7 d5d6 f8d6 c1g1 d6b4 g4h6 c4a2 c2d3 c7c3 d3c3 b4c3 h6f7 c3a1 f7d6 b7b8 g1a1",
        "go nodes 699041",
        "position startpos moves g1f3 g8f6 g2g3 g7g6 b2b3 f8g7 c1b2 e8g8 f1g2 d7d6 e1g1 e7e5 d2d3 h7h6 c2c4 f8e8 b1c3 c7c6 c3e4 f6e4 d3e4 a7a5 d1d2 b8a6 a1d1 a6c5 f3e1 d8b6 e1c2 g7f8 c2e3 h6h5 g1h1 a5a4 b3b4 a4a3 b2c1 c5d7 d2c3 a8a4 c1d2 d7f6 f2f3 b6d4 c3b3 d4a7 b3c2 h5h4 g3h4 f6h5 e3g4 c8e6 g2h3 b7b5 c4b5 c6b5 d2e3 a7e7 e3g5 f7f6 g5d2 g8h7 f1g1 e7f7 d1a1 e8c8 c2d3 e6c4 d3c2 c8c7 g1c1 d6d5 e4d5 a4a7 d2e3 a7b7 d5d6 f8d6 c1g1 d6b4 g4h6 c4a2 c2d3 c7c3 d3c3 b4c3 h6f7 c3a1 f7d6 b7b8 g1a1 b5b4 e3c5",
        "go nodes 1003308",
        "position startpos moves g1f3 g8f6 g2g3 g7g6 b2b3 f8g7 c1b2 e8g8 f1g2 d7d6 e1g1 e7e5 d2d3 h7h6 c2c4 f8e8 b1c3 c7c6 c3e4 f6e4 d3e4 a7a5 d1d2 b8a6 a1d1 a6c5 f3e1 d8b6 e1c2 g7f8 c2e3 h6h5 g1h1 a5a4 b3b4 a4a3 b2c1 c5d7 d2c3 a8a4 c1d2 d7f6 f2f3 b6d4 c3b3 d4a7 b3c2 h5h4 g3h4 f6h5 e3g4 c8e6 g2h3 b7b5 c4b5 c6b5 d2e3 a7e7 e3g5 f7f6 g5d2 g8h7 f1g1 e7f7 d1a1 e8c8 c2d3 e6c4 d3c2 c8c7 g1c1 d6d5 e4d5 a4a7 d2e3 a7b7 d5d6 f8d6 c1g1 d6b4 g4h6 c4a2 c2d3 c7c3 d3c3 b4c3 h6f7 c3a1 f7d6 b7b8 g1a1 b5b4 e3c5 a2g8 c5a7",
        "go nodes 1000000",
    };

    // uci_engine.process_uci_command("setoption name Hash value 8");
    // uci_engine.process_uci_command("setoption name Nonsense value true");
    // uci_engine.process_uci_command("setoption name SyzygyPath value C:/Users/nourb/OneDrive/Desktop/Chess_Engine/engine_executables/3-4-5_pieces_Syzygy/3-4-5");
    // std::this_thread::sleep_for(std::chrono::milliseconds(2000));

    // uci_engine.engine.load_state("saved_state.bin");

    std::vector<bool> correct_nodes = {};
    for (const std::string& input : commands) {
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

    // uci_engine.engine.save_state("saved_state.bin");

    std::cout << "==================" << std::endl;
    std::cout << "the engine did not stall" << std::endl;
    std::cout << "==================" << std::endl;

    for (int i = 0; i < correct_nodes.size() - 1; i++){
        if (!correct_nodes[i])
            std::cout << "missmatch between go nodes and command number: " << i << std::endl;
    }
    std::cin.get();
    return 0;
}



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
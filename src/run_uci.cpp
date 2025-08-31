#include "uci.hpp"
#include <iostream>
#include <string>
#include <random>

int main(int argc, char* argv[]){
    if (argc >= 2){
        if (std::string(argv[1]) == "bench"){
            Benchmark::benchmark_engine(BENCHMARK_DEPTH);
            return 0;
        }
            
        std::vector<std::string> parsed = UCIAgent::split_string(std::string(argv[1]));
        if (parsed.size() >= 4 && parsed[0] == "genfens"){
            int seed = std::stoi(parsed[3]);
            std::mt19937 rng(seed);

            Movelist move_list;
            Board board = Board();

            for (int i = 0; i < std::stoi(parsed[1]); i++){
                board.setFen(constants::STARTPOS);
                for (int i = 0; i < 10; i++){
                    if (std::get<1>(board.isGameOver()) != GameResult::NONE)
                        break;
                    movegen::legalmoves(move_list, board);
                    board.makeMove(move_list[rng()%move_list.size()]);
                }
                std::cout << "info string genfens " << board.getFen() << std::endl;
            }
            return 0;
        }
    }

    UCIAgent uci_engine = UCIAgent();
    std::string input;
    bool running;
    do {
        std::getline(std::cin, input);
        running = uci_engine.process_uci_command(input);
    } while (running);
    return 0;
}
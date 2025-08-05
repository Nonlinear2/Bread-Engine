#include "uci.hpp"
#include <iostream>
#include <string>

int main(int argc, char* argv[]){

    if (argc >= 2 && std::string(argv[1]) == "bench"){
        Benchmark::benchmark_engine(BENCHMARK_DEPTH);
        return 0;
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
#include "uci.hpp"
#include "datagen.hpp"
#include <iostream>
#include <string>
#include <random>

int main(int argc, char* argv[]){
    NNUE::init();

    UCIAgent uci_engine = UCIAgent();
    Engine& engine = uci_engine.workers.main().engine;

    if (argc >= 2){
        if (std::string(argv[1]) == "bench"){
            Benchmark::benchmark_engine(engine, BENCHMARK_DEPTH);
            return 0;
        }

        std::vector<std::string> parsed = split_string(std::string(argv[1]));
        if (parsed.size() >= 4 && parsed[0] == "genfens"){
            int seed = std::stoi(parsed[3]);
            std::mt19937 rng(seed);

            // silence engine
            engine.display_uci = false;
            Datagen::genfens(engine, rng, std::stoi(parsed[1]));
            engine.display_uci = true;

            if (argc >= 3 && std::string(argv[2]) == "quit")
                return 0;
        }
    }

    std::string input;
    bool running;
    do {
        std::getline(std::cin, input);
        running = uci_engine.process_uci_command(input);
    } while (running);

    NNUE::cleanup();
    return 0;
}
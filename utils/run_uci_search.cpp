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
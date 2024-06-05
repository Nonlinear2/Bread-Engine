#include "uci.hpp"
#include <iostream>

int main(){
    UCIAgent uci_engine = UCIAgent();
    std::string input;
    bool running;
    do {
        std::getline(std::cin, input);
        running = uci_engine.process_uci_command(input);
    } while (running);
    return 0;
}
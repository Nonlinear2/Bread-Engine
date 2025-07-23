#include "core.hpp"
#include "uci.hpp"
#include <iostream>
#include <fstream>
#include <chrono>
#include <thread>

int main(){
    UCIAgent uci_engine = UCIAgent();
    std::ifstream file(bread_DEBUG_UCI_COMMANDS_PATH);
    std::string input;
    bool running;

    if (!file.is_open()) {
        std::cerr << "failed to open file" << std::endl;
        return 1;
    }

    while (std::getline(file, input)) {
        std::cout << "-> " << input << std::endl;
        running = uci_engine.process_uci_command(input);
        if (UCIAgent::split_string(input)[0] == "go")
            // avoid writing logic to check if the thread is running
            std::this_thread::sleep_for(std::chrono::milliseconds(4000));
        if (!running)
            break;
    }

    if (uci_engine.main_search_thread.joinable()) {
        uci_engine.main_search_thread.join();
    }

    uci_engine.engine.save_state("saved_state.bin");
    std::cout << "ok\n";
    return 0;
}
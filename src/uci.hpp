#pragma once

#include <string>
#include <vector>
#include <iostream>
#include <sstream>
#include <thread>
#include "core.hpp"
#include "benchmark.hpp"

class UCIAgent {
    public:
    std::thread main_search_thread;
    int num_moves_out_of_book = 0;
    Engine engine = Engine();

    
    bool process_uci_command(std::string command);

    static std::vector<std::string> split_string(std::string str);

    private:
    int cached_think_time;
    
    void process_setoption(std::vector<std::string> command);
    
    void process_position(std::vector<std::string> command);

    void process_go(std::vector<std::string> command);

    void process_bench(std::vector<std::string> command);

    int get_think_time_from_go_command(std::vector<std::string> command);

    void interrupt_if_searching();
};

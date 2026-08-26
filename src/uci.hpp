#pragma once

#include <string>
#include <vector>
#include <deque>
#include <iostream>
#include <thread>
#include "tune.hpp"
#include "core.hpp"
#include "benchmark.hpp"
#include "thread.hpp"

class UCIAgent {
    public:
    UCIAgent();

    NnueBoard pos;
    TranspositionTable tt;
    std::atomic<int64_t> nodes;
    WorkerPool workers;

    bool process_uci_command(std::string command);

    private:
    int num_moves_out_of_book = 0;

    int cached_think_time;
    
    void process_setoption(std::vector<std::string> command);
    
    void process_position(std::vector<std::string> command);

    void process_go(std::vector<std::string> command);

    void process_bench(std::vector<std::string> command);

    void process_eval(std::vector<std::string> command);

    int get_think_time_from_go_command(std::vector<std::string> command);
};
#pragma once

#include <string>
#include <vector>
#include <iostream>
#include <thread>
#include "tune.hpp"
#include "core.hpp"
#include "benchmark.hpp"

struct Worker {
    public:
    Worker(bool is_main_thread, TranspositionTable& tt);
    std::thread thread;
    Engine engine;
};

class WorkerPool {
    public:
    WorkerPool(int size, TranspositionTable& tt);

    int size();
    void clear_state();
    void synchronize();
    void set_tablebase_loaded(bool tablebase_loaded);
    void set_is_nonsense(bool is_nonsense);
    void set_position(NnueBoard& pos);
    void update_limit(SearchLimit limit);

    void start_searching(SearchLimit limit);
    void interrupt_if_searching();

    Worker& main();

    private:
    std::vector<Worker> workers;
};

class UCIAgent {
    public:

    NnueBoard pos;
    TranspositionTable tt;
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
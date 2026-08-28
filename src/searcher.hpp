#pragma once

#include <string>
#include <vector>
#include <deque>
#include <iostream>
#include <thread>
#include "core.hpp"

struct Worker {
    public:
    Worker(bool is_main_thread, TranspositionTable& tt, WorkerPool& worker_pool);
    std::thread thread;
    Engine engine;
};

class WorkerPool {
    public:
    WorkerPool(int size, TranspositionTable& tt);

    int size();
    void set_size(int size);
    void clear_state();
    void synchronize();
    void set_tablebase_loaded(bool tablebase_loaded);
    void set_is_nonsense(bool is_nonsense);
    void set_position(NnueBoard& pos);
    void update_limit(SearchLimit limit);

    void start_searching(SearchLimit limit);
    void interrupt();
    void interrupt_and_join_threads();

    uint64_t total_node_count();

    Worker& main();

    private:
    TranspositionTable& tt;
    std::deque<Worker> workers;
};

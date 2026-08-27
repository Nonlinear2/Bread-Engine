#include "searcher.hpp"

Worker::Worker(bool is_main_thread, TranspositionTable& tt, std::atomic<int64_t>& nodes)
    : engine(is_main_thread, tt, nodes) {};

WorkerPool::WorkerPool(int size, TranspositionTable& tt, std::atomic<int64_t>& nodes){
    for (int i = 0; i < size; i++) {
        bool is_main = (i == 0);
        workers.emplace_back(is_main, tt, nodes);
    }
};

int WorkerPool::size(){
    return workers.size();
}

void WorkerPool::clear_state(){
    main().engine.tt.clear(size()); // tt is shared between all threads
    for (auto& worker: workers)
        worker.engine.clear_state();
}

void WorkerPool::synchronize(){
    for (auto& worker: workers)
        worker.engine.pos.synchronize();
}

void WorkerPool::set_tablebase_loaded(bool tablebase_loaded){
    for (auto& worker: workers)
        worker.engine.tablebase_loaded = tablebase_loaded;
}

void WorkerPool::set_is_nonsense(bool is_nonsense){
    for (auto& worker: workers)
        worker.engine.is_nonsense = is_nonsense;
}

void WorkerPool::set_position(NnueBoard& pos){
    for (auto& worker: workers)
        worker.engine.pos = pos;
}

void WorkerPool::update_limit(SearchLimit limit){
    for (auto& worker: workers)
        worker.engine.limit.store(limit);
};

void WorkerPool::start_searching(SearchLimit limit){
    for (auto& worker: workers)
        worker.thread = std::thread(&Engine::iterative_deepening, &worker.engine, limit);
}

void WorkerPool::interrupt_and_join_threads(){
    for (auto& worker: workers)
        if (worker.thread.joinable()){
            worker.engine.interrupt_flag = true;
            worker.thread.join();
            worker.engine.interrupt_flag = false;
        }
}

Worker& WorkerPool::main(){
    return workers[0];
}

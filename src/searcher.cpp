#include "searcher.hpp"

Worker::Worker(bool is_main_thread, TranspositionTable& tt, std::atomic<int64_t>& nodes)
    : engine(is_main_thread, tt, nodes) {};

WorkerPool::WorkerPool(int size, TranspositionTable& tt, std::atomic<int64_t>& nodes)
    : tt(tt), nodes(nodes) {
    set_size(size);
}

void WorkerPool::set_size(int size){
    interrupt_and_join_threads();
    workers.clear();

    for (int i = 0; i < size; i++) {
        bool is_main = (i == 0);
        workers.emplace_back(is_main, tt, nodes);
    }
};

int WorkerPool::size(){
    return workers.size();
}

void WorkerPool::clear_state(){
    interrupt_and_join_threads(); // make sure data races can't happen

    main().engine.tt.clear(size()); // tt is shared between all threads
    for (auto& worker: workers)
        worker.engine.clear_state();
}

void WorkerPool::synchronize(){
    interrupt_and_join_threads(); // make sure data races can't happen

    for (auto& worker: workers)
        worker.engine.pos.synchronize();
}

void WorkerPool::set_tablebase_loaded(bool tablebase_loaded){
    interrupt_and_join_threads(); // make sure data races can't happen

    for (auto& worker: workers)
        worker.engine.tablebase_loaded = tablebase_loaded;
}

void WorkerPool::set_is_nonsense(bool is_nonsense){
    interrupt_and_join_threads(); // make sure data races can't happen

    for (auto& worker: workers)
        worker.engine.is_nonsense = is_nonsense;
}

void WorkerPool::set_position(NnueBoard& pos){
    interrupt_and_join_threads(); // make sure data races can't happen

    for (auto& worker: workers)
        worker.engine.pos = pos; 
}

void WorkerPool::update_limit(SearchLimit limit){
    for (auto& worker: workers)
        worker.engine.limit.store(limit);
};

void WorkerPool::start_searching(SearchLimit limit){
    interrupt_and_join_threads(); // make sure the engine isn't running

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


uint64_t WorkerPool::total_node_count(){
    return nodes.load(std::memory_order::relaxed);
}

Worker& WorkerPool::main(){
    return workers[0];
}

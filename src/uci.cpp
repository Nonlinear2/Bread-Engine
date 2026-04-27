#include "uci.hpp"

Worker::Worker(bool is_main_thread, TranspositionTable& tt): engine(is_main_thread, tt) {};

WorkerPool::WorkerPool(int size, TranspositionTable& tt){
    for (int i = 0; i < size; i++) {
        bool is_main = (i == 0);
        workers.emplace_back(is_main, tt);
    }
};

int WorkerPool::size(){
    return workers.size();
}

void WorkerPool::clear_state(){
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

void WorkerPool::interrupt_if_searching(){
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

UCIAgent::UCIAgent(): workers(1, tt) {};

bool UCIAgent::process_uci_command(std::string command){
    std::vector<std::string> parsed_command = split_string(command);
    std::string first = parsed_command[0];

    if (first == "uci"){
        std::cout << "id name Bread Engine " << bread_VERSION << std::endl;
        std::cout << "id author Nonlinear" << std::endl;
        std::cout << std::endl;
        std::cout << "option name SyzygyPath type string default <empty>" << std::endl;
        std::cout << "option name Hash type spin default 256 min " << TT_MIN_SIZE << " max " << TT_MAX_SIZE << std::endl;
        std::cout << "option name Threads type spin default 1 min 1 max 1024" << std::endl;
        std::cout << "option name Nonsense type check default false" << std::endl;
        // spsa tune options
        auto& tuneables = SPSA::get_values();
        for (const auto& [name, info] : tuneables) {
            std::cout << "option name " << name << " type spin default " << *info.val_ptr
                      << " min " << info.min << " max " << info.max << std::endl;
        }

        std::cout << "uciok" << std::endl;
    } else if (first == "setoption"){
        process_setoption(parsed_command);

    } else if (first == "isready"){
        std::cout << "readyok" << std::endl;

    } else if (first == "ucinewgame"){
        pos.setFen(constants::STARTPOS);
        workers.clear_state();

    } else if (first == "position"){
        process_position(parsed_command);
    
    } else if (first == "bench"){
        process_bench(parsed_command);

    } else if (first == "eval"){
        process_eval(parsed_command);

    } else if (first == "go"){
        workers.interrupt_if_searching();
        process_go(parsed_command);

    } else if (first == "ponderhit"){
        workers.update_limit(SearchLimit(LimitType::Time, cached_think_time));

    } else if (first == "stop"){
        workers.interrupt_if_searching();

    } else if (first == "quit"){
        workers.interrupt_if_searching();
        tb_free();
        return 0;
    } else {
        std::cout << "unrecognized command: " << command << "\n";
    }

    return true;
}

void UCIAgent::process_setoption(std::vector<std::string> command){
    assert(command.size() >= 5); // setoption name ... value ...
    std::string option_name = command[2];
    std::string option_value = command[4];

    if (option_name == "SyzygyPath"){
        std::string path = option_value;
        // handle path containing spaces
        for (int i = 5; i < command.size(); i++)
            path += " " + command[i];

        bool tb_success = tb_init(path.c_str());
        if (!tb_success){
            workers.set_tablebase_loaded(false);
            std::cout << "info string tablebase initialisation failed" << std::endl;
        } else if (TB_LARGEST == 0){
            workers.set_tablebase_loaded(false);
            std::cout << "info string no tablebase loaded" << std::endl;
        } else {
            workers.set_tablebase_loaded(true);
            std::cout << "info string tablebase loaded" << std::endl;
        }
    } else if (option_name == "Hash"){
        int size = std::stoi(option_value);
        if ((size & (size - 1)) == 0){
            tt.allocateMB(size);
            std::cout << "info string hash size set to " << size << std::endl;
        } else {
            std::cout << "info string hash size must be a power of 2" << std::endl;
        }
    } else if (option_name == "Threads"){
        workers = WorkerPool(std::stoi(option_value), tt);
        std::cout << "info string number of threads set to " << workers.size() << std::endl;
    } else if (option_name == "Nonsense"){
        workers.set_is_nonsense(option_value == "true");
        std::cout << "info string nonsense " << (option_value == "true" ? "activated" : "deactivated") << std::endl;
    } else if (is_number_string(option_value) && SPSA::is_registered(option_name)){
        SPSA::set_value(option_name, std::stoi(option_value));
    }
}

void UCIAgent::process_position(std::vector<std::string> command){
    workers.interrupt_if_searching();

    std::string fen = "";
    if (command[1] == "startpos"){
        fen = constants::STARTPOS;
    } else if (command[1] == "fen") {
        for (int i = 2; i < command.size(); i++){
            if (i > 2) fen += ' ';
            fen += command[i];
        }
    }

    pos.setFen(fen);

    bool is_movelist = false; 
    for (int i = 2; i < command.size(); i++){
        std::string token = command[i];
        if (is_movelist)
            pos.makeMove(uci::uciToMove(pos, token));

        if (token == "moves")
            is_movelist = true;
    }

    workers.set_position(pos);
}

void UCIAgent::process_bench(std::vector<std::string> command){
    if (command.size() == 1)
        Benchmark::benchmark_engine(workers.main().engine, BENCHMARK_DEPTH);
    else if (command[1] == "nn")
        Benchmark::benchmark_nn();
}

void UCIAgent::process_eval(std::vector<std::string> command){
    pos.synchronize();
    int score = pos.sideToMove() == Color::BLACK ? -1 : 1;
    score *= pos.evaluate();
    std::cout << "static evaluation: " << (score >= 0 ? "+" : "") << score << std::endl;
}

void UCIAgent::process_go(std::vector<std::string> command){
    std::string go_type = command[1];

    SearchLimit limit;
    if (go_type == "ponder"){
        cached_think_time = get_think_time_from_go_command(command);
        limit = SearchLimit(LimitType::Depth, ENGINE_MAX_DEPTH);
    } else if (go_type == "movetime"){
        limit = SearchLimit(LimitType::Time, std::stoi(command[2]));
    } else if (go_type == "depth"){
        limit = SearchLimit(LimitType::Depth, std::stoi(command[2]));
    } else if (go_type == "nodes"){
        limit = SearchLimit(LimitType::Nodes, std::stoi(command[2]));
    } else if (go_type == "infinite"){
        limit = SearchLimit(LimitType::Depth, ENGINE_MAX_DEPTH);
    } else {
        limit = SearchLimit(LimitType::Time, get_think_time_from_go_command(command));
    }

    workers.start_searching(limit);
}

int UCIAgent::get_think_time_from_go_command(std::vector<std::string> command){
    num_moves_out_of_book++;

    int wtime = -1;
    int btime = -1;
    int winc = 0;
    int binc = 0;
    int movestogo = 0;
    for (int i = 1; i < command.size(); i++){
        std::string token = command[i];
        std::string next_token = command[i+1];
        if (token == "wtime")
            wtime = std::stoi(next_token);
        else if (token == "btime")
            btime = std::stoi(next_token);
        else if (token == "winc")
            winc = std::stoi(next_token);
        else if (token == "binc")
            binc = std::stoi(next_token);
        else if (token == "movestogo")
            movestogo = std::stoi(next_token);
    };

    bool engine_white = (pos.sideToMove() == Color::WHITE);
    return get_think_time(engine_white ? wtime: btime, 
                          num_moves_out_of_book,
                          movestogo,
                          engine_white ? winc: binc);
}
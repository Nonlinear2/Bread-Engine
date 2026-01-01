#include "uci.hpp"

bool UCIAgent::process_uci_command(std::string command){
    std::vector<std::string> parsed_command = split_string(command);
    std::string first = parsed_command[0];

    if (first == "uci"){
        std::cout << "id name Bread Engine " << bread_VERSION << std::endl;
        std::cout << "id author Nonlinear" << std::endl;
        std::cout << std::endl;
        std::cout << "option name SyzygyPath type string default <empty>" << std::endl;
        std::cout << "option name Hash type spin default 256 min " << TT_MIN_SIZE << " max " << TT_MAX_SIZE << std::endl;
        std::cout << "option name Threads type spin default 1 min 1 max 1" << std::endl;
        std::cout << "option name Nonsense type check default false" << std::endl;
        // spsa tune options
        auto& tuneables = SPSA::get_values();
        for (const auto& pair : tuneables) {
            std::cout << "option name " << pair.first << " type spin default " << *pair.second << std::endl;
        }

        std::cout << "uciok" << std::endl;
    } else if (first == "setoption"){
        process_setoption(parsed_command);

    } else if (first == "isready"){
        std::cout << "readyok" << std::endl;

    } else if (first == "ucinewgame"){
        engine.pos.setFen(constants::STARTPOS);
        engine.clear_state();

    } else if (first == "position"){
        process_position(parsed_command);
    
    } else if (first == "bench"){
        process_bench(parsed_command);

    } else if (first == "evaluate"){
        process_evaluate(parsed_command);

    } else if (first == "go"){
        interrupt_if_searching();
        process_go(parsed_command);

    } else if (first == "ponderhit"){
        engine.limit.store(SearchLimit(LimitType::Time, cached_think_time));

    } else if (first == "stop"){
        interrupt_if_searching();

    } else if (first == "quit"){
        interrupt_if_searching();
        tb_free();
        return 0;
    } else {
        std::cout << "unrecognized command: " << command << "\n";
    }

    return true;
}

std::vector<std::string> UCIAgent::split_string(std::string str) {
    std::stringstream ss(str);
    std::string curr;
    std::vector<std::string> split;

    while (ss >> curr)
        split.push_back(curr);

    return split;
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
            engine.tablebase_loaded = false;
            std::cout << "info string tablebase initialisation failed" << std::endl;
        } else if (TB_LARGEST == 0){
            engine.tablebase_loaded = false;
            std::cout << "info string no tablebase loaded" << std::endl;
        } else {
            engine.tablebase_loaded = true;
            std::cout << "info string tablebase loaded" << std::endl;
        }
    } else if (option_name == "Hash"){
        int size = std::stoi(option_value);
        if ((size & (size - 1)) == 0){
            size = std::clamp(size, 2, 4096);
            engine.transposition_table.allocateMB(size);
            std::cout << "info string hash size set to " << size << std::endl;
        } else {
            std::cout << "info string hash size must be a power of 2" << std::endl;
        }
    } else if (option_name == "Threads"){
       std::cout << "info string number of threads set to 1" << std::endl;
    } else if (option_name == "Nonsense"){
        bool value = (option_value == "true");
        engine.is_nonsense = value;
        std::cout << "info string nonsense " << (value ? "activated" : "deactivated") << std::endl;
    } else if (is_number_string(option_value) && SPSA::is_registered(option_name)){
        SPSA::set_value(option_name, std::stoi(option_value));
    }
}

void UCIAgent::process_position(std::vector<std::string> command){
    interrupt_if_searching();

    std::string fen = "";
    if (command[1] == "startpos"){
        fen = constants::STARTPOS;
    } else if (command[1] == "fen") {
        for (int i = 2; i < command.size(); i++){
            if (i > 2) fen += ' ';
            fen += command[i];
        }
    }

    engine.pos.setFen(fen);

    bool is_movelist = false; 
    for (int i = 2; i < command.size(); i++){
        std::string token = command[i];
        if (is_movelist)
            engine.pos.makeMove(uci::uciToMove(engine.pos, token));

        if (token == "moves")
            is_movelist = true;
    }
}

void UCIAgent::process_bench(std::vector<std::string> command){
    if (command.size() == 1)
        Benchmark::benchmark_engine(engine, BENCHMARK_DEPTH);
    else if (command[1] == "nn")
        Benchmark::benchmark_nn();
}

void UCIAgent::process_evaluate(std::vector<std::string> command){
    engine.pos.synchronize();
    int score = engine.pos.sideToMove() == Color::BLACK ? -1 : 1;
    score *= engine.pos.evaluate();
    std::string sign = score >= 0 ? "+" : "";
    std::cout << "NNUE Evaluation: " << sign << score << std::endl;
}

void UCIAgent::process_go(std::vector<std::string> command){
    std::string go_type = command[1];

    if (go_type == "ponder"){
        cached_think_time = get_think_time_from_go_command(command);
        if (cached_think_time == -1) return; // error occured
        main_search_thread = std::thread(&Engine::iterative_deepening,
            &engine, SearchLimit(LimitType::Depth, ENGINE_MAX_DEPTH));
        return;
    } else if (go_type == "movetime"){
        main_search_thread = std::thread(&Engine::iterative_deepening,
            &engine, SearchLimit(LimitType::Time, std::stoi(command[2])));
        return;
    } else if (go_type == "depth"){
        main_search_thread = std::thread(&Engine::iterative_deepening,
            &engine, SearchLimit(LimitType::Depth, std::stoi(command[2])));
        return;
    } else if (go_type == "nodes"){
        main_search_thread = std::thread(&Engine::iterative_deepening,
            &engine, SearchLimit(LimitType::Nodes, std::stoi(command[2])));
        return;
    } else if (go_type == "infinite"){
        main_search_thread = std::thread(&Engine::iterative_deepening,
            &engine, SearchLimit(LimitType::Depth, ENGINE_MAX_DEPTH));
        return;
    } else {
        int think_time = get_think_time_from_go_command(command);
        if (think_time == -1)
            return; // error occured
        main_search_thread = std::thread(&Engine::iterative_deepening,
            &engine, SearchLimit(LimitType::Time, think_time));
    }
}

int UCIAgent::get_think_time_from_go_command(std::vector<std::string> command){
    num_moves_out_of_book++;

    int wtime = -1;
    int btime = -1;
    int winc = 0;
    int binc = 0;
    int movestogo = 0;
    for (int i=1; i < command.size(); i++){
        std::string token = command[i];
        if (token == "wtime")
            wtime = std::stoi(command[i+1]);
        else if (token == "btime")
            btime = std::stoi(command[i+1]);
        else if (token == "winc")
            winc = std::stoi(command[i+1]);
        else if (token == "binc")
            binc = std::stoi(command[i+1]);
        else if (token == "movestogo")
            movestogo = std::stoi(command[i+1]);
    };

    if ((wtime == -1) || (btime == -1)){
        std::cout << "no time specified\n";
        return -1;
    }

    bool engine_color = (engine.pos.sideToMove() == Color::WHITE);
    return engine.get_think_time(engine_color ? wtime: btime, 
                                 num_moves_out_of_book,
                                 movestogo,
                                 engine_color ? winc: binc);
}

void UCIAgent::interrupt_if_searching(){
    if (main_search_thread.joinable()){
        engine.interrupt_flag = true;
        main_search_thread.join();
        engine.interrupt_flag = false;
    }
}
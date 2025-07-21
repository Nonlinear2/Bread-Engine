#include "uci.hpp"

bool UCIAgent::process_uci_command(std::string command){
    std::vector<std::string> parsed_command = split_string(command);
    std::string first = parsed_command[0];

    if (first == "uci"){
        std::cout << "id name Bread Engine " << bread_VERSION << std::endl;
        std::cout << "id author Nonlinear" << std::endl;
        std::cout << std::endl;
        std::cout << "option name SyzygyPath type string default <empty>" << std::endl;
        std::cout << "option name hash type spin default 256 min " << TT_MIN_SIZE << " max " << TT_MAX_SIZE << std::endl;
        std::cout << "option name nonsense type check default false" << std::endl;
        std::cout << "uciok" << std::endl;
    } else if (first == "setoption"){
        process_setoption(parsed_command);

    } else if (first == "isready"){
        std::cout << "readyok" << std::endl;

    } else if (first == "ucinewgame"){
        engine.transposition_table.clear();
        SortedMoveGen<movegen::MoveGenType::ALL>::history.clear();
        SortedMoveGen<movegen::MoveGenType::ALL>::cont_history.clear();
    } else if (first == "position"){
        process_position(parsed_command);
    
    } else if (first == "bench"){
        process_bench(parsed_command);

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

std::vector<std::string> UCIAgent::split_string(std::string str){
    std::vector<std::string> split;
    std::string current = "";
    
    for (int i = 0; i < str.length(); i++){
        if (str[i] != ' ') {
            current += str[i];
        } else if (!current.empty()) {
            split.push_back(current);
            current = "";
        }
    }

    if (!current.empty()) {
        split.push_back(current);
    }
    
    return split;
}

void UCIAgent::process_setoption(std::vector<std::string> command){
    std::string option_name = command[2];
    std::transform(option_name.begin(), option_name.end(), option_name.begin(), [](unsigned char c){ return std::tolower(c); });
    if (option_name == "syzygypath"){
        std::string path = command[4];
        for (int i = 5; i < command.size(); i++){
            path += " " + command[i];
        }
        bool tb_success = tb_init(path.c_str());
        if (!tb_success){
            std::cout << "info string tablebase initialisation failed" << std::endl;
        } else if (TB_LARGEST == 0){
            std::cout << "info string no tablebase loaded" << std::endl;
        } else {
            std::cout << "info string tablebase loaded" << std::endl;
        }
    } else if (option_name == "hash"){
        int size = std::stoi(command[4]);
        if ((size & (size - 1)) == 0){
            size = std::clamp(size, 2, 4096);
            engine.transposition_table.allocateMB(size);
            std::cout << "info string hash size set to " << size << std::endl;
        } else {
            std::cout << "info string hash size must be a power of 2" << std::endl;
        }
    } else if (option_name == "nonsense"){
        bool value = (command[4] == "true");
        engine.is_nonsense = value;
        std::cout << "info string nonsense " << (value ? "activated" : "deactivated") << std::endl;
    }
}

void UCIAgent::process_position(std::vector<std::string> command){
    interrupt_if_searching();

    std::string fen = "";
    if (command[1] == "startpos"){
        fen = constants::STARTPOS;
    } else if (command[1] == "fen") {
        for (int i=0; i < 5; i++){
            fen += command[2+i] + " ";
        }
        fen += command[7];
    }
    engine.pos.setFen(fen);

    bool is_movelist = false; 
    for (int i=2; i < command.size(); i++){
        std::string token = command[i];
        if (is_movelist){
            engine.pos.makeMove(uci::uciToMove(engine.pos, token));
        }
        if (token == "moves"){
            is_movelist = true;
        }
    }
}

void UCIAgent::process_bench(std::vector<std::string> command){
    if (command.size() == 1){
        Benchmark::benchmark_engine(BENCHMARK_DEPTH);
    } else if (command[1] == "nn"){
        Benchmark::benchmark_nn();
    }
}

void UCIAgent::process_go(std::vector<std::string> command){
    if (command.size() < 3){
        std::cout << "incorrect syntax" << std::endl;
        return;
    }

    std::string go_type = command[1];

    if (go_type == "ponder"){
        cached_think_time = get_think_time_from_go_command(command);
        if (cached_think_time == -1) return; // error occured
        main_search_thread = std::thread(&Engine::iterative_deepening, &engine, SearchLimit(LimitType::Depth, ENGINE_MAX_DEPTH));
        return;
    } else if (go_type == "movetime"){
        main_search_thread = std::thread(&Engine::iterative_deepening, &engine, SearchLimit(LimitType::Time, std::stoi(command[2])));
        return;
    } else if (go_type == "depth"){
        main_search_thread = std::thread(&Engine::iterative_deepening, &engine, SearchLimit(LimitType::Depth, std::stoi(command[2])));
        return;
    } else if (go_type == "nodes"){
        main_search_thread = std::thread(&Engine::iterative_deepening, &engine, SearchLimit(LimitType::Nodes, std::stoi(command[2])));
        return;
    } else if (go_type == "infinite"){
        main_search_thread = std::thread(&Engine::iterative_deepening, &engine, SearchLimit(LimitType::Depth, ENGINE_MAX_DEPTH));
        return;
    } else {
        int think_time = get_think_time_from_go_command(command);
        if (think_time == -1)
            return; // error occured
        main_search_thread = std::thread(&Engine::iterative_deepening, &engine, SearchLimit(LimitType::Time, think_time));
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
        if (token == "wtime"){
            wtime = std::stoi(command[i+1]);
        } else if (token == "btime"){
            btime = std::stoi(command[i+1]);
        } else if (token == "winc"){
            winc = std::stoi(command[i+1]);
        } else if (token == "binc"){
            binc = std::stoi(command[i+1]);
        } else if (token == "movestogo"){
            movestogo = std::stoi(command[i+1]);
        }
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
#include "uci.hpp"

bool UCIAgent::process_uci_command(std::string command){
    std::vector<std::string> parsed_command = split_string(command);
    std::string first = parsed_command[0];

    if (first == "uci"){
        std::cout << "id name Bread Engine\n";
        std::cout << "id author Nonlinear\n";
        std::cout << "uciok\n";

    } else if (first == "isready"){
        std::cout << "readyok\n";

    } else if (first == "ucinewgame"){
        engine.transposition_table.clear();
        
    } else if (first == "position"){
        process_position(parsed_command);

    } else if (first == "go"){
        interrupt_if_searching();
        process_go(parsed_command);

    } else if (first == "ponderhit"){
        engine.time_limit = think_time;

    } else if (first == "stop"){
        interrupt_if_searching();

    } else if (first == "quit"){
        interrupt_if_searching();
        #if bread_USE_TB
        tb_free();
        #endif
        return 0;

    } else {
        std::cout << "unrecognized" << command << "\n";
    }

    return true;
}

std::vector<std::string> UCIAgent::split_string(std::string str){
    std::vector<std::string> split;
    int head = 0;
    for (int i = 0; i < str.length()+1; i++){
        if ((str[i] == ' ') | (i == str.length())){
            split.push_back(str.substr(head, i-head));
            head = i + 1;
        }
    }
    return split;
}

void UCIAgent::process_position(std::vector<std::string> command){
    interrupt_if_searching();

    std::string fen = "";
    if (command[1] == "startpos"){
        fen = chess::constants::STARTPOS;
    } else if (command[1] == "fen") {
        for (int i=0; i < 5; i++){
            fen += command[2+i] + " ";
        }
        fen += command[7];
    }
    engine.inner_board.setFen(fen);

    bool is_movelist = false; 
    for (int i=2; i < command.size(); i++){
        std::string token = command[i];
        if (is_movelist){
            engine.inner_board.makeMove(chess::uci::uciToMove(engine.inner_board, token));
        }
        if (token == "moves"){
            is_movelist = true;
        }
    }
}

void UCIAgent::process_go(std::vector<std::string> command){
    int wtime = -1;
    int btime = -1;
    int inc = 0;
    int movestogo = 0;
    for (int i=1; i < command.size(); i++){
        std::string token = command[i];
        if (token == "wtime"){
            wtime = std::stoi(command[i+1]);
        } else if (token == "btime"){
            btime = std::stoi(command[i+1]);
        } else if ((token == "winc") | (token == "binc")){
            inc = std::stoi(command[i+1]);
        } else if (token == "movestogo"){
            movestogo = std::stoi(command[i+1]);
            // normally, handle depth, node, mate, movetime command now
        } else if (token == "infinite"){
            wtime = btime = INT32_MAX;
        }
    }
    if ((wtime == -1) || (btime == -1)){
        std::cout << "no time specified\n";
        return;
    };

    int engine_time_left = (engine.inner_board.sideToMove() == chess::Color::WHITE) ? wtime: btime;

    think_time = static_cast<int>(engine.get_think_time(engine_time_left, num_moves_out_of_book, movestogo, inc));
    if (command[1] == "ponder"){
        main_search_thread = std::thread(&Engine::iterative_deepening, 
                                         &engine, INT32_MAX, 0, 25); // infinite search time
    } else {
        num_moves_out_of_book++;
        main_search_thread = std::thread(&Engine::iterative_deepening, 
                                         &engine, think_time, 0, 25);
    }
}

void UCIAgent::interrupt_if_searching(){
    if (main_search_thread.joinable()){
        engine.set_interrupt_flag();
        main_search_thread.join();
        engine.unset_interrupt_flag();
    }
}
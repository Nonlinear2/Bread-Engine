#include "bread_engine_core.hpp"

inline std::vector<std::string> fens = {
    "1k1r4/pp1b1R2/3q2pp/4p3/2B5/4Q3/PPP2B2/2K5 b - -",
    "3r1k2/4npp1/1ppr3p/p6P/P2PPPP1/1NR5/5K2/2R5 w - -",
    "2q1rr1k/3bbnnp/p2p1pp1/2pPp3/PpP1P1P1/1P2BNNP/2BQ1PRK/7R b - -",
    "rnbqkb1r/p3pppp/1p6/2ppP3/3N4/2P5/PPP1QPPP/R1B1KB1R w KQkq -",
    "r1b2rk1/2q1b1pp/p2ppn2/1p6/3QP3/1BN1B3/PPP3PP/R4RK1 w - -",
    "2r3k1/pppR1pp1/4p3/4P1P1/5P2/1P4K1/P1P5/8 w - -",
    "1nk1r1r1/pp2n1pp/4p3/q2pPp1N/b1pP1P2/B1P2R2/2P1B1PP/R2Q2K1 w - -",
    "4b3/p3kp2/6p1/3pP2p/2pP1P2/4K1P1/P3N2P/8 w - -",
    "2kr1bnr/pbpq4/2n1pp2/3p3p/3P1P1B/2N2N1Q/PPP3PP/2KR1B1R w - -",
    "3rr1k1/pp3pp1/1qn2np1/8/3p4/PP1R1P2/2P1NQPP/R1B3K1 b - -",
    "2r1nrk1/p2q1ppp/bp1p4/n1pPp3/P1P1P3/2PBB1N1/4QPPP/R4RK1 w - -",
    "r3r1k1/ppqb1ppp/8/4p1NQ/8/2P5/PP3PPP/R3R1K1 b - -",
    "r2q1rk1/4bppp/p2p4/2pP4/3pP3/3Q4/PP1B1PPP/R3R1K1 w - -",
    "rnb2r1k/pp2p2p/2pp2p1/q2P1p2/8/1Pb2NP1/PB2PPBP/R2Q1RK1 w - -",
    "2r3k1/1p2q1pp/2b1pr2/p1pp4/6Q1/1P1PP1R1/P1PN2PP/5RK1 w - -",
    "r1bqkb1r/4npp1/p1p4p/1p1pP1B1/8/1B6/PPPN1PPP/R2Q1RK1 w kq -",
    "r2q1rk1/1ppnbppp/p2p1nb1/3Pp3/2P1P1P1/2N2N1P/PPB1QP2/R1B2RK1 b - -",
    "r1bq1rk1/pp2ppbp/2np2p1/2n5/P3PP2/N1P2N2/1PB3PP/R1B1QRK1 b - -",
    "3rr3/2pq2pk/p2p1pnp/8/2QBPP2/1P6/P5PP/4RRK1 b - -",
    "r4k2/pb2bp1r/1p1qp2p/3pNp2/3P1P2/2N3P1/PPP1Q2P/2KRR3 w - -",
    "3rn2k/ppb2rpp/2ppqp2/5N2/2P1P3/1P5Q/PB3PPP/3RR1K1 w - -",
    "2r2rk1/1bqnbpp1/1p1ppn1p/pP6/N1P1P3/P2B1N1P/1B2QPP1/R2R2K1 b - -",
    "r1bqk2r/pp2bppp/2p5/3pP3/P2Q1P2/2N1B3/1PP3PP/R4RK1 b kq -",
    "r2qnrnk/p2b2b1/1p1p2pp/2pPpp2/1PP1P3/PRNBB3/3QNPPP/5RK1 w - -",
};

int sum(std::vector<int> values){
    int total = 0;
    for (auto v: values){
        total += v;
    }
    return total;
}

void benchmark_nn(){
    std::cout << "============================== \n";
    std::cout << "evaluation function benchmark: \n";

    NnueBoard board = NnueBoard();

    auto start = std::chrono::high_resolution_clock::now();
    int num_iter = 10'000;
    for (int i = 0; i < num_iter; i++){
        board.evaluate();
    }
    int mean = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - start).count()/num_iter;
    std::cout << "time taken: " << mean << " microseconds per call";
    std::cout << "============================== \n";
}

void benchmark_engine(int depth){
    Engine engine = Engine();

    std::vector<int> times;

    chess::Board cb = chess::Board();

    for (auto fen: fens){
        std::chrono::time_point<std::chrono::high_resolution_clock> start_time = std::chrono::high_resolution_clock::now();
        engine.search(fen, SearchLimit(LimitType::Depth, depth));
        times.push_back(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - start_time).count());
        

        cb.setFen(fen);

        // print info to terminal
        std::cout << engine.current_depth << std::endl;
    }
    engine.transposition_table.info();

    std::cout << "============================== \n";
    std::cout << "average time: " << sum(times)/fens.size() << "\n";
    std::cout << "============================== \n";
}

void benchmark_engine_nodes(int depth){
    Engine engine = Engine();

    std::vector<int> nodes;
    
    chess::Board cb = chess::Board();
    for (auto fen: fens){

        engine.search(fen, SearchLimit(LimitType::Depth, depth));
        nodes.push_back(engine.nodes);

        cb.setFen(fen);

        // print info to terminal
        std::cout << engine.current_depth << std::endl;
    }
    engine.transposition_table.info();
    int avg_nodes = sum(nodes)/fens.size();
    std::cout << "============================== \n";
    std::cout << "average nodes: " << avg_nodes << "\n";
    std::cout << "============================== \n";
}

int main(){
    benchmark_engine(10);
    return 0;
}

// average nodes: 5.54152e+06
// depth 9: average time: 2613.78

// with improved pst: average nodes: 5.76197e+06

// 5.76678e+06

// 5.6313e+06

// GCC: average time: 2800.05, average time: 2820.78
// clang: average time: 2462.3, average time: 2386.76
// msvc: average time: 2662.35, average time: 2694.83


// msvc new avx add:  2730.59,  2633.28

// clang average time: 2266

// tt improvements: clang average time 2048, 2065

// no tt imrpveements: average time: 2083.56, average time: 2037.84

// less probe calls when at boundary between negamax and qsearch
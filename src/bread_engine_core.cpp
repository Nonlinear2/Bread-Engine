#include "bread_engine_core.hpp"

bool Engine::try_outcome_eval(int& eval){
    // chess::GameResult outcome = inner_board.isGameOver().second;
    
    // we dont want history dependent data to be stored in the TT.
    // the evaluation stored in the TT should only depend on the
    // position on the board and not how we got there, otherwise these evals would be reused
    // incorrectly.
    // here, threefold repetition is already handled and resulting evals
    // are not stored in the TT.
    // the fifty move rule is not yet supported
    if (inner_board.isInsufficientMaterial()){
        eval = 0;
        return true;
    }

    chess::Movelist movelist;
    chess::movegen::legalmoves(movelist, inner_board);

    if (movelist.empty()){
        // checkmate/stalemate.
        eval = inner_board.inCheck() ? -MATE_EVAL : 0;
        return true;
    }
    return false;
}

chess::Move Engine::minimax_root(int depth, int color){
    int alpha = WORST_EVAL;
    int beta = BEST_EVAL;
    chess::Move best_move = NO_MOVE;
    uint64_t zobrist_hash = inner_board.hash();
    inner_board.synchronize();

    SortedMoveGen sorted_move_gen = SortedMoveGen<chess::movegen::MoveGenType::ALL>(inner_board, depth);
    bool is_hit;
    TEntry* transposition = transposition_table.probe(is_hit, zobrist_hash);
    if (is_hit){
        if ((transposition->depth() >= depth) && (!inner_board.isRepetition(1))){
            // if is repetition(1), danger of repetition so TT is unreliable
            switch (transposition->flag()){
                case TFlag::EXACT:
                    best_move = transposition->best_move;
                    best_move.setScore(transposition->evaluation);
                    return best_move;
                default:
                    break;
            }
        }
        if (transposition->best_move != NO_MOVE){
            // in case the search breaks early, initialize best_move.
            best_move = transposition->best_move;
            best_move.setScore(transposition->evaluation);
            sorted_move_gen.set_tt_move(transposition->best_move);
        }
    }

    sorted_move_gen.generate_moves();
    
    int pos_eval;
    chess::Move move;
    while(sorted_move_gen.next(move)){
        bool is_capture = inner_board.isCapture(move);

        inner_board.update_state(move);

        int new_depth = depth-1;
        new_depth += inner_board.inCheck();
        new_depth -= ((sorted_move_gen.index() > 1) && (depth > 5) && (!is_capture) && (!inner_board.inCheck()));

        if (sorted_move_gen.index() == 0){
            pos_eval = -negamax<true>(new_depth, -color, -beta, -alpha);
        } else {
            pos_eval = -negamax<false>(new_depth, -color, -beta, -alpha);
            if ((new_depth < depth-1) && (pos_eval > alpha)){
                pos_eval = -negamax<false>(depth-1, -color, -beta, -alpha);
            }
        }

        inner_board.restore_state(move);

        if (interrupt_flag){
            if (best_move != NO_MOVE){
                return best_move; 
            } else { // this means no move was ever recorded
                std::cerr << "error: the engine didn't complete any search." << std::endl;
                return NO_MOVE;
            }
        }
        
        if (is_mate(pos_eval)) pos_eval = increment_mate_ply(pos_eval);

        move.setScore(pos_eval);

        if (pos_eval > alpha){
            best_move = move;
            alpha = pos_eval;
        }
    }
    
    transposition_table.store(zobrist_hash, alpha, depth, best_move, TFlag::EXACT, static_cast<uint8_t>(inner_board.fullMoveNumber()));

    return best_move;
}

// to make the engine prefer faster checkmates instead of stalling,
// we decrease the eval if the checkmate is deeper in the search tree.

int Engine::increment_mate_ply(int eval){
    return (is_win(eval) ? 1 : -1)*(std::abs(eval) - 1);
}

bool Engine::is_mate(int eval){
    return (std::abs(eval) >= 80'000);
}

bool Engine::is_win(int eval){
    return (eval >= 80'000);
}

int Engine::get_mate_in_moves(int eval){
    int ply = -std::abs(eval) + MATE_EVAL;
    return (is_win(eval) ? 1: -1)*(ply/2 + (ply%2 != 0)); 
}

int Engine::get_think_time(int time_left, int num_moves_out_of_book, int num_moves_until_time_control=0, int increment=0){
    float target;
    float move_num = num_moves_out_of_book < 10 ? static_cast<float>(num_moves_out_of_book) : 10;
    float factor = 2 -  move_num / 10;
    if (num_moves_until_time_control == 0){
        target = static_cast<float>(time_left) / 30;
    } else {
        target = static_cast<float>(time_left) / (num_moves_until_time_control+5);
    }
    return static_cast<int>(factor*target + 0.9F*increment);
}

std::pair<std::string, std::string> Engine::get_pv_pmove(std::string fen){
    std::string pv = "";
    std::string ponder_move = "";
    chess::Board pv_visitor;

    pv_visitor.setFen(fen);

    for (int i = 0; i < current_depth; i++){
        bool is_hit;
        TEntry* transposition = transposition_table.probe(is_hit, pv_visitor.hash());
        if ((!is_hit) || (transposition->best_move == NO_MOVE)){
            break;
        }
        if (i == 1){
            ponder_move = chess::uci::moveToUci(transposition->best_move);
        }
        pv += " " + chess::uci::moveToUci(transposition->best_move);
        pv_visitor.makeMove(transposition->best_move);
    }
    return std::pair(pv, ponder_move);
}

chess::Move Engine::search(std::string fen, SearchLimit limit){
    inner_board.setFen(fen);
    return iterative_deepening(limit);
};

chess::Move Engine::search(SearchLimit limit){
    return iterative_deepening(limit);
};

chess::Move Engine::iterative_deepening(SearchLimit limit){
    
    this->limit = limit;
    start_time = std::chrono::high_resolution_clock::now();

    std::string initial_fen = inner_board.getFen();

    std::string pv;
    std::string ponder_move = "";

    chess::Move best_move = NO_MOVE;

    SortedMoveGen<chess::movegen::MoveGenType::ALL>::clear_killer_moves();
    // SortedMoveGen<chess::movegen::MoveGenType::ALL>::history.clear();


    nodes = 0;

    current_depth = 0;

    engine_color = (inner_board.sideToMove() == chess::Color::WHITE) ? 1: -1;

    chess::Move tb_move;
    if (inner_board.probe_dtz(tb_move)){
        update_run_time();
        std::cout << "info depth 0";
        std::cout << " score cp " << tb_move.score();
        std::cout << " nodes 0 nps 0";
        std::cout << " time " << run_time;
        std::cout << " hashfull " << transposition_table.hashfull();
        std::cout << " pv " << chess::uci::moveToUci(tb_move) << std::endl;
        std::cout << "bestmove " << chess::uci::moveToUci(tb_move) << std::endl;
        return tb_move;
    };

    while (true){
        current_depth++;

        best_move = minimax_root(current_depth, engine_color);

        std::pair<std::string, std::string> pv_pmove = get_pv_pmove(initial_fen);
        pv = pv_pmove.first;
        if (pv_pmove.second.size() > 0){
            ponder_move = pv_pmove.second;
        }
        update_run_time();
        if (run_time == 0) run_time = 1; // avoid division by 0;
        // do not count interrupted searches in depth
        std::cout << "info depth " << current_depth - interrupt_flag;
        if (is_mate(best_move.score())){
            std::cout << " score mate " << get_mate_in_moves(best_move.score()); 
        } else {
            std::cout << " score cp " << static_cast<int>(std::tanh(static_cast<float>(best_move.score())/(64*127))*1111);
        }
        std::cout << " nodes " << nodes;
        std::cout << " nps " << nodes*1000/run_time;
        std::cout << " time " << run_time;
        std::cout << " hashfull " << transposition_table.hashfull();
        std::cout << " pv" << pv << std::endl;
        

        // should the search really stop if there is a mate for the oponent?
        if ((interrupt_flag) ||
            (is_mate(best_move.score())) ||
            ((limit.type == LimitType::Depth) && (current_depth == limit.value)) ||
            (current_depth == ENGINE_MAX_DEPTH)) break;
        
    }

    std::cout << "bestmove " << chess::uci::moveToUci(best_move);
    if (ponder_move.size() > 0){
        std::cout << " ponder " << ponder_move;
    }
    std::cout << std::endl;
    interrupt_flag = false;
    return best_move;
}

bool Engine::update_interrupt_flag(){
    SearchLimit limit_ = limit.load();
    switch (limit_.type){
        case LimitType::Time:
            update_run_time();
            interrupt_flag = (run_time >= limit_.value);
            break;
        case LimitType::Nodes:
            interrupt_flag = (nodes >= limit_.value);
            break;
        default:
            interrupt_flag = false;
            break;
    }
    return interrupt_flag;
}

void Engine::update_run_time(){
    run_time = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - start_time).count();
};

template<bool pv>
int Engine::negamax(int depth, int color, int alpha, int beta){
    assert((alpha < BEST_EVAL) && (beta > WORST_EVAL));
    nodes++;
    // we check can_return only at depth 5 or higher to avoid doing it at all nodes
    if (interrupt_flag || ((depth >= 5) && update_interrupt_flag())){
        return 0; // the value doesn't matter, it won't be used.
    }

    // there are no repetition checks in qsearch as captures can never lead to repetition.
    if (inner_board.isRepetition(2)){
        return 0;
    }

    // transpositions will be checked inside of qsearch
    // if isRepetition(1), qsearch will not consider the danger of draw as it searches captures.
    if (depth == 0){
        return qsearch<pv>(alpha, beta, color, QSEARCH_MAX_DEPTH);
    }

    const int initial_alpha = alpha;
    uint64_t zobrist_hash = inner_board.hash();

    SortedMoveGen sorted_move_gen = SortedMoveGen<chess::movegen::MoveGenType::ALL>(inner_board, depth);
    bool is_hit;
    TEntry* transposition = transposition_table.probe(is_hit, zobrist_hash);
    if (is_hit){
        if ((transposition->depth() >= depth) && (!inner_board.isRepetition(1))){
            // if is repetition(1), danger of repetition so TT is unreliable
            switch (transposition->flag()){
                case TFlag::EXACT:
                    return transposition->evaluation;
                case TFlag::LOWER_BOUND:
                    alpha = std::max(alpha, transposition->evaluation);
                    break;
                case TFlag::UPPER_BOUND:
                    beta = std::min(beta, transposition->evaluation);
                    break;
                default:
                    break;
            }
            // we need to do a cutoff check because we updated alpha/beta.
            if (beta <= alpha){
                // no need to store in transposition table as is already there.
                return transposition->evaluation;
            }
        }

        if (transposition->best_move != NO_MOVE) sorted_move_gen.set_tt_move(transposition->best_move);
    }

    int static_eval = is_hit ? transposition->evaluation : inner_board.evaluate();
    // reverse futility pruning
    if (!pv && (depth < 5) && (static_eval - depth*800 - 1500 >= beta)){
        return beta;
    }

    // null move pruning
    // maybe check for zugzwang?
    int null_move_eval;
    if (!pv && 
        (depth > 4) &&
        !inner_board.inCheck() && 
        !inner_board.last_move_null() && 
        beta != BEST_EVAL)
    {
        int R = 2 + (static_eval >= beta);
        inner_board.makeNullMove();
        null_move_eval = -negamax<false>(depth - R, -color, -beta, -beta+1);
        inner_board.unmakeNullMove();
        if (null_move_eval >= beta) return beta;
    }

    sorted_move_gen.generate_moves();
    int max_eval = WORST_EVAL;

    if (sorted_move_gen.is_empty()){ // avoid calling expensive try_outcome_eval function
        // If board is in check, it is checkmate
        // if there are no legal moves and it's not check, it is stalemate so eval is 0
        max_eval = inner_board.inCheck() ? -MATE_EVAL : 0;
        // we know this eval is exact at any depth, but 
        // we also don't want this eval to pollute the transposition table.
        // the full move number will make sure it is replaced at some point.
        transposition_table.store(zobrist_hash, max_eval, 255, NO_MOVE, TFlag::EXACT, static_cast<uint8_t>(inner_board.fullMoveNumber()));
        return max_eval;
    }

    if (inner_board.isHalfMoveDraw()) return 0;

    chess::Move best_move;
    chess::Move move;
    int pos_eval;
    while (sorted_move_gen.next(move)){
        bool is_capture = inner_board.isCapture(move);
        inner_board.update_state(move);
        
        // late move reductions:
        // depth > 2 is to make sure the new depth is not less than 0.
        // (!inner_board.inCheck()) is to see if the move gives check. 
        // (we already updated the inner board so we only need to check if it is check)
        bool is_killer = SortedMoveGen<chess::movegen::MoveGenType::ALL>::killer_moves[depth].in_buffer(move);
        int new_depth = depth-1;
        new_depth += inner_board.inCheck();
        new_depth -= ((sorted_move_gen.index() > 1) && (depth > 2) && (!is_capture) && (!inner_board.inCheck()) && (!is_killer));
        new_depth -= (depth > 5) && (!is_hit) && (!is_killer); // IIR /// depth > 3

        if (pv && (sorted_move_gen.index() == 0)){
            pos_eval = -negamax<true>(new_depth, -color, -beta, -alpha);
        } else {
            pos_eval = -negamax<false>(new_depth, -color, -beta, -alpha);
            if ((new_depth < depth-1) && (pos_eval > alpha)){
                pos_eval = -negamax<false>(depth-1, -color, -beta, -alpha);
            }
        }

        inner_board.restore_state(move);
        
        if (interrupt_flag) return 0;

        if (pos_eval > max_eval){
            max_eval = pos_eval;
            best_move = move;
        }

        alpha = std::max(alpha, pos_eval);
        if (beta <= alpha){
            if (!is_capture) sorted_move_gen.update_history(move, depth, color);
            SortedMoveGen<chess::movegen::MoveGenType::ALL>::killer_moves[depth].add_move(move);
            break;
        }
    }

    // if max eval was obtained because of a threefold repetition,
    // the eval should not be stored in the transposition table, as
    // this is a history dependent evaluation.
    // what if max eval was not obtained because of a threefold? in this case we loose
    // one TT entry which is completely fine.
    // what about evaluations higher in the tree where inner_board.isRepetition(1) is false?
    // these evals are not history dependent as they mean that one side can force a draw.
    if (((max_eval == 0) && (inner_board.isRepetition(1))) || (inner_board.halfMoveClock() + depth >= 100)){
        return max_eval;
        // we fall through without storing the eval in the TT.
    };

    TFlag node_type;
    if (beta <= alpha){ // this means there was a cutoff. So true eval is equal or higher
        node_type = TFlag::LOWER_BOUND;
    } else if (max_eval <= initial_alpha){
        // this means that the evals were all lower than best option for us
        // so it is possible there was a beta cutoff. So eval is equal or lower
        node_type = TFlag::UPPER_BOUND;
    } else {
        // eval is between initial alpha and beta. so search was completed without cutoffs, and is exact
        node_type = TFlag::EXACT;
    }
    if (is_mate(max_eval)) max_eval = increment_mate_ply(max_eval);
    transposition_table.store(zobrist_hash, max_eval, depth, best_move, node_type, static_cast<uint8_t>(inner_board.fullMoveNumber()));

    return max_eval;
}

template<bool pv>
int Engine::qsearch(int alpha, int beta, int color, int depth){
    // assert(pv || ((alpha == (beta-1)) && (alpha == (beta-1))));
    nodes++;

    int stand_pat;
    
    // tablebase probe
    if (inner_board.probe_wdl(stand_pat)){
        return stand_pat;
    }

    // this is recomputed when qsearch is called the first time. Performance loss is probably low. 
    uint64_t zobrist_hash = inner_board.hash();

    // first we check for transposition. If it is outcome, it should have already been stored
    // with an exact flag, so the stand pat will be correct anyways.
    SortedMoveGen sorted_capture_gen = SortedMoveGen<chess::movegen::MoveGenType::CAPTURE>(inner_board);
    bool is_hit;
    TEntry* transposition = transposition_table.probe(is_hit, zobrist_hash);
    if (is_hit){
        switch (transposition->flag()){
            case TFlag::EXACT:
                stand_pat = transposition->evaluation;
                if (transposition->depth() == 0){ // outcomes are stored at depth 255
                    // we can already check for cutoff
                    if (stand_pat >= beta) return stand_pat;
                } else {
                    return stand_pat;
                }
                break;
            case TFlag::LOWER_BOUND:
                if (!pv) alpha = std::max(alpha, transposition->evaluation);
                is_hit = false;
                break;
            case TFlag::UPPER_BOUND:
                if (!pv) beta = std::min(beta, transposition->evaluation); 
                is_hit = false;
                break;
            default:
                break;
        }
        if (beta <= alpha){
            // no need to store in transposition table as is already there.
            return transposition->evaluation;
        }
        if (transposition->best_move != NO_MOVE) sorted_capture_gen.set_tt_move(transposition->best_move);
    }

    if (depth != 0){
        sorted_capture_gen.generate_moves();
    }

    if (!is_hit){
        // if it is hit, no need to check for outcome, as it wouldn't be stored in the
        // transposition table at a 0 depth. 
        if (sorted_capture_gen.is_empty() && try_outcome_eval(stand_pat)){ // only generate non captures?
            transposition_table.store(zobrist_hash, stand_pat, 255, NO_MOVE, TFlag::EXACT, static_cast<uint8_t>(inner_board.fullMoveNumber()));
            return stand_pat;
        }

        if (inner_board.isHalfMoveDraw()) return 0;

        stand_pat = inner_board.evaluate();
        if (stand_pat >= beta){
            transposition_table.store(zobrist_hash, stand_pat, 0, NO_MOVE, TFlag::EXACT, static_cast<uint8_t>(inner_board.fullMoveNumber()));
            return stand_pat;
        }
    }   

    if (depth == 0){
        transposition_table.store(zobrist_hash, stand_pat, 0, NO_MOVE, TFlag::EXACT, static_cast<uint8_t>(inner_board.fullMoveNumber()));
        return stand_pat;
    }

    alpha = std::max(alpha, stand_pat);

    int max_eval = stand_pat;
    int pos_eval;
    chess::Move move;
    chess::Move best_move = NO_MOVE;
    while (sorted_capture_gen.next(move)){
        // delta pruning
        // move.score() is calculated with set_capture_score which is material difference.
        // 7300 is the equivalent of a queen, as a safety margin
        if (stand_pat + move.score()*800 + 8000 < alpha){ // multiplication by 733 is to convert from pawn to "engine" centipawns.
            continue;
        }

        inner_board.update_state(move);
        pos_eval = -qsearch<pv>(-beta, -alpha, -color, depth-1);
        inner_board.restore_state(move);

        if (pos_eval > max_eval){
            max_eval = pos_eval;
            best_move = move;
        }
        alpha = std::max(alpha, pos_eval);
        if (alpha >= beta){ // only check for cutoffs when alpha gets updated.
            break;
        }
    }
    if (inner_board.halfMoveClock() + depth >= 100) return max_eval; // avoid storing history dependant evals.
    transposition_table.store(zobrist_hash, stand_pat, 0, best_move, TFlag::EXACT, static_cast<uint8_t>(inner_board.fullMoveNumber()));
    
    if (is_mate(max_eval)) max_eval = increment_mate_ply(max_eval);
    return max_eval;
}
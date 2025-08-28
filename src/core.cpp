#include "core.hpp"


UNACTIVE_TUNEABLE(r_1, int, 176, 0, 500, 20, 0.002);
UNACTIVE_TUNEABLE(r_2, int, 274, -100, 1000, 25, 0.002);
UNACTIVE_TUNEABLE(rfp_1, int, 207, 0, 500, 25, 0.002);
UNACTIVE_TUNEABLE(rfp_2, int, 200, 0, 1000, 50, 0.002);
UNACTIVE_TUNEABLE(rfp_3, int, 139, -100, 1000, 20, 0.002);
UNACTIVE_TUNEABLE(nmp_1, int, 82, -50, 250, 10, 0.002);
UNACTIVE_TUNEABLE(nmp_2, int, 15, -300, 300, 15, 0.002);
UNACTIVE_TUNEABLE(lmp_1, int, 106, -100, 500, 20, 0.002);
UNACTIVE_TUNEABLE(see_1, int, 135, -100, 1000, 25, 0.002);
UNACTIVE_TUNEABLE(see_2, int, 10, 0, 100, 0.5, 0.002);
UNACTIVE_TUNEABLE(se_1, int, 9, -100, 100, 1, 0.002);
UNACTIVE_TUNEABLE(se_2, int, 5, -100, 100, 1, 0.002);
UNACTIVE_TUNEABLE(lmr_1, int, 9, 0, 23, 0.5, 0.002);
UNACTIVE_TUNEABLE(cont_1, int, 881, 0, 3000, 70, 0.002);
UNACTIVE_TUNEABLE(cont_2, int, 199, 0, 1500, 35, 0.002);
UNACTIVE_TUNEABLE(qs_fp_1, int, 1657, 0, 3000, 70, 0.002);
UNACTIVE_TUNEABLE(qs_see_1, int, 298, 0, 900, 25, 0.002);
UNACTIVE_TUNEABLE(qs_p_1, int, 1003, 0, 3000, 50, 0.002);
UNACTIVE_TUNEABLE(qs_p_2, int, 166, 0, 900, 20, 0.002);

void Engine::set_uci_display(bool v){
    display_uci = v;
}

int Engine::get_think_time(float time_left, int num_moves_out_of_book, int num_moves_until_time_control=0, int increment=0){
    float move_num = num_moves_out_of_book < 10 ? static_cast<float>(num_moves_out_of_book) : 10;
    float factor = 2 -  move_num / 10;
    float target = num_moves_until_time_control == 0
        ? time_left / 30
        : time_left / (num_moves_until_time_control+5);
    
    return static_cast<int>(factor*target + 0.9F*increment);
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

void Engine::clear_state(){
    transposition_table.clear();
    SortedMoveGen<movegen::MoveGenType::ALL>::history.clear();
    SortedMoveGen<movegen::MoveGenType::ALL>::cont_history.clear();
    SortedMoveGen<movegen::MoveGenType::ALL>::killer_moves.clear();
}

void Engine::save_state(std::string file){
    std::ofstream ofs(file, std::ios::binary | std::ios::out);
    if (!ofs) {
        std::cout << "Could not open file for writing." << std::endl;
        return;
    }

    transposition_table.save_to_stream(ofs);
    SortedMoveGen<movegen::MoveGenType::ALL>::history.save_to_stream(ofs);
    SortedMoveGen<movegen::MoveGenType::ALL>::cont_history.save_to_stream(ofs);
    SortedMoveGen<movegen::MoveGenType::ALL>::killer_moves.save_to_stream(ofs);

    ofs.close();
}

void Engine::load_state(std::string file){
    std::ifstream ifs(file, std::ios::binary | std::ios::in);
    if (!ifs) {
        std::cout << "Could not open file for reading." << std::endl;
    }

    transposition_table.load_from_stream(ifs);
    SortedMoveGen<movegen::MoveGenType::ALL>::history.load_from_stream(ifs);
    SortedMoveGen<movegen::MoveGenType::ALL>::cont_history.load_from_stream(ifs);
    SortedMoveGen<movegen::MoveGenType::ALL>::killer_moves.load_from_stream(ifs);

    ifs.close();
}

void Engine::update_run_time(){
    run_time = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::high_resolution_clock::now() - start_time
    ).count();
};

std::pair<std::string, std::string> Engine::get_pv_pmove(){
    std::string pv = "";
    std::string ponder_move = "";

    Board pv_visitor = pos;

    for (int i = 0; i < current_depth; i++){
        bool is_hit;
        TTData transposition = transposition_table.probe(is_hit, pv_visitor.hash());
        if (transposition.move == Move::NO_MOVE || pv_visitor.isRepetition(2)
            || pv_visitor.isHalfMoveDraw() || pv_visitor.isInsufficientMaterial())
            break;

        if (i == 1)
            ponder_move = uci::moveToUci(transposition.move);

        pv += " " + uci::moveToUci(transposition.move);
        pv_visitor.makeMove(transposition.move);
    }
    return std::pair(pv, ponder_move);
}

Move Engine::search(std::string fen, SearchLimit limit){
    pos.setFen(fen);
    return iterative_deepening(limit);
};

Move Engine::search(SearchLimit limit){
    return iterative_deepening(limit);
};

Move Engine::iterative_deepening(SearchLimit limit){
    if (is_nonsense){
        srand((unsigned int)time(NULL));
        if (nonsense.should_bongcloud(pos.hash(), pos.fullMoveNumber()))
            return nonsense.play_bongcloud(display_uci);
    }

    this->limit = limit;
    start_time = std::chrono::high_resolution_clock::now();

    std::string pv;
    std::string ponder_move = "";

    Move best_move = Move::NO_MOVE;

    SortedMoveGen<movegen::MoveGenType::ALL>::killer_moves.clear();

    nodes = 0;
    current_depth = 0;

    root_moves.clear();

    // initialize stack
    for (int i = 0; i < SEARCH_STACK_SIZE + STACK_PADDING_SIZE; i++){
        stack[i] = Stack();
    }

    Move tb_move;
    Movelist tb_moves;
    if (pos.probe_root_dtz(tb_move, tb_moves, is_nonsense)){
        update_run_time();
        if (display_uci){
            std::cout << "info depth 0";
            std::cout << " score cp " << tb_move.score();
            std::cout << " nodes 0 nps 0";
            std::cout << " time " << run_time;
            std::cout << " hashfull " << transposition_table.hashfull();
        }
        if (is_nonsense && tb_move.score() == TB_VALUE){
            tb_move = nonsense.worst_winning_move(tb_move, tb_moves);
            tb_move.setScore(TB_VALUE);
            if (display_uci){
                std::cout << " pv " << uci::moveToUci(tb_move) << std::endl;
                std::cout << "bestmove " << uci::moveToUci(tb_move) << std::endl;
            }
            return tb_move;
        }
        if (display_uci){
            std::cout << " pv " << uci::moveToUci(tb_move) << std::endl;
            std::cout << "bestmove " << uci::moveToUci(tb_move) << std::endl;
        }
        return tb_move;
    };

    while (true){
        current_depth++;

        negamax<true>(current_depth, -INFINITE_VALUE, INFINITE_VALUE, root_ss);
        best_move = root_moves[0];

        std::pair<std::string, std::string> pv_pmove = get_pv_pmove();
        pv = pv_pmove.first;
        if (pv_pmove.second.size() > 0){
            ponder_move = pv_pmove.second;
        }

        update_run_time();
        if (run_time == 0)
            run_time = 1; // avoid division by 0;

        if (display_uci){
            // do not count interrupted searches in depth
            std::cout << "info depth " << current_depth - interrupt_flag;
            if (is_mate(best_move.score())){
                std::cout << " score mate " << get_mate_in_moves(best_move.score()); 
            } else {
                std::cout << " score cp " << best_move.score();
            }
            std::cout << " nodes " << nodes;
            std::cout << " nps " << nodes * 1000 / run_time;
            std::cout << " time " << run_time;
            std::cout << " hashfull " << transposition_table.hashfull();
            std::cout << " pv" << pv << std::endl;
        }

        // should the search really stop if there is a mate for the oponent?
        if (interrupt_flag
            || is_mate(best_move.score())
            || (limit.type == LimitType::Depth && current_depth == limit.value)
            || current_depth >= ENGINE_MAX_DEPTH)
            break;
    }

    if (is_nonsense && display_uci)
        nonsense.display_info();

    if (display_uci){
        std::cout << "bestmove " << uci::moveToUci(best_move);
        if (ponder_move.size() > 0)
            std::cout << " ponder " << ponder_move;
        std::cout << std::endl;
    }
    interrupt_flag = false;
    return best_move;
}

template<bool pv>
int Engine::negamax(int depth, int alpha, int beta, Stack* ss){
    assert(ss - stack < SEARCH_STACK_SIZE); // avoid stack overflow
    assert(alpha < INFINITE_VALUE && beta > -INFINITE_VALUE);
    assert(depth <= ENGINE_MAX_DEPTH);

    nodes++;

    const bool root_node = ss == root_ss;
    assert(!root_node || pos.isGameOver().second == GameResult::NONE);

    if (root_node)
        pos.synchronize();

    // we check can_return only at depth 5 or higher to avoid doing it at all nodes
    if (interrupt_flag || (depth >= 5 && update_interrupt_flag()))
        return NO_VALUE; // the value doesn't matter, it won't be used.

    // a stalemate will be processed after the move generation
    if (pos.isRepetition(2) || pos.isHalfMoveDraw() || pos.isInsufficientMaterial())
        return 0;

    if (ss - stack >= SEARCH_STACK_SIZE - 1)
        return pos.evaluate();

    // transpositions will be checked inside of qsearch
    // if isRepetition(1), qsearch will not consider the danger of draw as it searches captures.
    if (depth <= 0)
        return qsearch<pv>(alpha, beta, 0, ss + 1);

    int static_eval, eval;
    int max_value = -INFINITE_VALUE;
    Move best_move = Move::NO_MOVE;
    Move move;
    int value;

    const int initial_alpha = alpha;
    uint64_t zobrist_hash = pos.hash();

    SortedMoveGen move_gen = SortedMoveGen<movegen::MoveGenType::ALL>(
        root_node ? &root_moves : NULL, (ss - 1)->moved_piece, (ss - 1)->current_move.to().index(), pos, depth
    );

    if (root_node && root_moves.empty()){
        movegen::legalmoves(root_moves, pos);
        assert(!root_moves.empty());

        move_gen.prepare_pos_data();
        for (int i = 0; i < root_moves.size(); i++)
            move_gen.set_score(root_moves[i]);

        std::stable_sort(root_moves.begin(), root_moves.end(),
            [](const Move a, const Move b){ return a.score() > b.score(); });

        for (Move& m: root_moves)
            m.setScore(NO_VALUE);
    }

    bool is_hit;
    TTData transposition = transposition_table.probe(is_hit, zobrist_hash);
    
    static_eval = eval = transposition.static_eval;
    eval = transposition.value;
    move_gen.set_tt_move(transposition.move);
    
    chess::Move excluded_move = ss->excluded_move;
    if (!root_node && !pv && transposition.depth >= depth && excluded_move == Move::NO_MOVE && !pos.isRepetition(1)){
        switch (transposition.flag){
            case TFlag::EXACT:
                return transposition.value;
            case TFlag::LOWER_BOUND:
                alpha = std::max(alpha, transposition.value);
                break;
            case TFlag::UPPER_BOUND:
                beta = std::min(beta, transposition.value);
                break;
            default:
                break;
        }
    }

    // we need to do a cutoff check because we updated alpha/beta.
    // no need to store in transposition table as is already there.
    if (beta <= alpha)
        return transposition.value;

    bool in_check = pos.inCheck();

    if (static_eval == NO_VALUE)
        static_eval = pos.evaluate();
    if (eval == NO_VALUE)
        eval = static_eval;

    ss->static_eval = static_eval;

    bool improving = is_valid(ss->static_eval) && is_valid((ss - 2)->static_eval)
        && ss->static_eval > (ss - 2)->static_eval;

    // pruning
    if (!root_node && !pv && !in_check){

        // razoring
        if (eval + r_1*depth*depth + r_2 < alpha){ 
            eval = qsearch<false>(alpha, beta, 0, ss + 1); // we update static eval to the better qsearch eval.
            if (eval <= alpha)
                return eval;
        }

        // reverse futility pruning
        if (depth < 6 && eval - depth*rfp_1 - rfp_2 + rfp_3*improving >= beta)
            return eval;

        // null move pruning
        // maybe check for zugzwang?
        int null_move_eval;
        if (!pos.last_move_null() && excluded_move == Move::NO_MOVE
            && eval > beta - depth*nmp_1 + nmp_2 && is_regular_eval(beta)){

            int R = 2 + (eval >= beta) + depth / 4;
            ss->moved_piece = Piece::NONE;
            ss->current_move = Move::NULL_MOVE;
            pos.makeNullMove();
            null_move_eval = -negamax<false>(depth - R, -beta, -beta + 1, ss + 1);
            pos.unmakeNullMove();
            if (null_move_eval >= beta && !is_win(null_move_eval))
                return null_move_eval;
        }
    }

    bool tt_capture = transposition.move != Move::NO_MOVE && pos.isCapture(transposition.move);
    while (move_gen.next(move)){
        bool is_capture = pos.isCapture(move);

        if (move == excluded_move)
            continue;
            
        bool is_killer = SortedMoveGen<movegen::MoveGenType::ALL>::killer_moves.in_buffer(depth, move);

        if (!root_node && is_valid(max_value) && !is_loss(max_value)){
            // lmp
            if (!pv && !in_check && !is_capture && move_gen.index() > 2 + depth + improving 
                && !is_hit && eval - lmp_1 * !improving < alpha)
                continue;

            // SEE pruning
            if (!pv && !in_check && !is_capture && !is_killer && move_gen.index() > 5 + depth / 2
                && depth < 5 && !SEE::evaluate(pos, move, alpha - static_eval - see_1 - see_2*depth))
                continue;
        }
        
        int new_depth = depth-1;
    
        // singular extensions
        int extension = 0;
        if (!root_node && is_hit && is_regular_eval(transposition.value)
            && move == transposition.move && excluded_move == Move::NO_MOVE
            && depth >= 6 && (transposition.flag == TFlag::LOWER_BOUND || transposition.flag == TFlag::EXACT)
            && transposition.depth >= depth - 1)
        {
            int singular_beta = transposition.value - se_1 - se_2*depth;

            if (is_regular_eval(singular_beta)){
                ss->excluded_move = move;
                value = negamax<false>(new_depth / 2, singular_beta - 1, singular_beta, ss);
                ss->excluded_move = Move::NO_MOVE;
    
                if (interrupt_flag)
                    return NO_VALUE;
    
                if (value < singular_beta)
                    extension = 1;
                else if (value >= beta && !is_decisive(value))
                    return value;
            }
        }

        new_depth += extension;

        ss->moved_piece = pos.at(move.from());
        ss->current_move = move;
        pos.update_state(move);

        bool gives_check = pos.inCheck();

        // late move reductions
        new_depth += gives_check && !root_node;
        new_depth -= move_gen.index() > 1 && !is_capture && !gives_check && !is_killer;
        new_depth -= depth > 5 && !is_hit && !is_killer; // IIR
        new_depth -= tt_capture && !is_capture;
        new_depth -= move_gen.index() > lmr_1;
        new_depth -= move_gen.index() > 8 && depth > 7 && !improving;

        new_depth = std::min(new_depth, ENGINE_MAX_DEPTH);

        if (pv && (move_gen.index() == 0)){
            value = -negamax<true>(new_depth, -beta, -alpha, ss + 1);
        } else {
            value = -negamax<false>(new_depth, -beta, -alpha, ss + 1);
            if ((new_depth < depth-1) && (value > alpha)){
                value = -negamax<false>(depth-1, -beta, -alpha, ss + 1);
                if (!is_capture)
                    move_gen.update_cont_history(ss->moved_piece, move.to().index(), cont_1);
            }
            else if (value < alpha && !is_capture)
                move_gen.update_cont_history(ss->moved_piece, move.to().index(), -cont_2);
        }

        pos.restore_state(move);

        if (interrupt_flag)
            return NO_VALUE;

        if (root_node)
            root_moves[move_gen.index()].setScore(value);

        if (value > max_value){
            max_value = value;
            if (value > alpha)
                best_move = move;
            if (root_node){
                // ! This preserves the order of the array after the current move.
                // ! Rotate invalidates root_moves[move_gen.index()].
                std::rotate(root_moves.begin(), root_moves.begin() + move_gen.index(),
                    root_moves.begin() + move_gen.index() + 1);
            }
        }

        alpha = std::max(alpha, value);
        if (beta <= alpha){
            if (!is_capture)
                move_gen.update_history(move, depth);
            SortedMoveGen<movegen::MoveGenType::ALL>::killer_moves.add_move(depth, move);
            break;
        }
    }

    if (!is_valid(max_value)){
        assert(!root_node);
        if (move_gen.tt_move == Move::NO_MOVE && move_gen.empty()){ // avoid calling expensive try_outcome_eval function
            // If board is in check, it is checkmate
            // if there are no legal moves and it's not check, it is stalemate so eval is 0
            max_value = pos.inCheck() ? -MATE_VALUE : 0;
            // we know this eval is exact at any depth, but 
            // we also don't want this eval to pollute the transposition table.
            // the full move number will make sure it is replaced at some point.
            transposition_table.store(zobrist_hash, max_value, NO_VALUE, 255, Move::NO_MOVE, TFlag::EXACT, static_cast<uint8_t>(pos.fullMoveNumber()));
            return max_value;
        }

        // if the move gen is not empty, the only legal move must be the excluded move
        assert(excluded_move != Move::NO_MOVE);
        assert(move_gen.tt_move == excluded_move);
        assert(move_gen.index() == 1);

        // we return a fail low to extend the search by 1
        return alpha;
    }

    // if max eval was obtained because of a threefold repetition,
    // the eval should not be stored in the transposition table, as
    // this is a history dependent evaluation.
    // what if max eval was not obtained because of a threefold? in this case we loose
    // one TT entry which is completely fine.
    // what about evaluations higher in the tree where pos.isRepetition(1) is false?
    // these evals are not history dependent as they mean that one side can force a draw.
    if (!root_node && ((max_value == 0 && pos.isRepetition(1)) || pos.halfMoveClock() + depth >= 100))
        return max_value;
    // we fall through without storing the eval in the TT.

    TFlag node_type;
    if (beta <= alpha){ // this means there was a cutoff. So true eval is equal or higher
        node_type = TFlag::LOWER_BOUND;
    } else if (max_value <= initial_alpha){
        // this means that the evals were all lower than best option for us
        // So eval is equal or lower
        node_type = TFlag::UPPER_BOUND;
    } else {
        // eval is between initial alpha and beta. so search was completed without cutoffs, and is exact
        node_type = TFlag::EXACT;
    }

    assert(is_valid(max_value));

    if (is_mate(max_value))
        max_value = increment_mate_ply(max_value);

    transposition_table.store(zobrist_hash, max_value, static_eval, depth, best_move, node_type, static_cast<uint8_t>(pos.fullMoveNumber()));

    return max_value;
}

template<bool pv>
int Engine::qsearch(int alpha, int beta, int depth, Stack* ss){
    assert(ss - stack < SEARCH_STACK_SIZE); // avoid stack overflow
    // assert(pv || ((alpha == (beta-1)) && (alpha == (beta-1))));
    nodes++;

    int stand_pat = NO_VALUE;

    // tablebase probe
    if (pos.probe_wdl(stand_pat))
        return stand_pat;

    if (pos.isHalfMoveDraw()) 
        return 0;

    int value;
    Move move;
    Move best_move = Move::NO_MOVE;

    // this is recomputed when qsearch is called the first time. Performance loss is probably low. 
    uint64_t zobrist_hash = pos.hash();

    // first we check for transposition. If it is outcome, it should have already been stored
    // with an exact flag, so the stand pat will be correct anyways.
    SortedMoveGen capture_gen = SortedMoveGen<movegen::MoveGenType::CAPTURE>(
        (ss - 1)->moved_piece, (ss - 1)->current_move.to().index(), pos
    );
    
    bool is_hit;
    TTData transposition = transposition_table.probe(is_hit, zobrist_hash);

    if (!pv && is_valid(transposition.value)){
        switch (transposition.flag){
            case TFlag::EXACT:
                return transposition.value;
            case TFlag::LOWER_BOUND:
                alpha = std::max(alpha, transposition.value);
                break;
            case TFlag::UPPER_BOUND:
                beta = std::min(beta, transposition.value);
                break;
            default:
                break;
        }

        if (beta <= alpha)
            return transposition.value;
    }

    stand_pat = transposition.static_eval;

    if (!is_valid(stand_pat))
        stand_pat = pos.evaluate();

    assert(is_regular_eval(stand_pat, false));

    if (is_valid(transposition.value) && !is_decisive(transposition.value)
        && (
            transposition.flag == TFlag::EXACT 
            || (transposition.flag == TFlag::LOWER_BOUND && transposition.value >= stand_pat)
            || (transposition.flag == TFlag::UPPER_BOUND && transposition.value <= stand_pat)
            ))
            stand_pat = transposition.value;
        
    if (stand_pat >= beta){
        if (!is_hit)
            transposition_table.store(zobrist_hash, stand_pat, stand_pat, DEPTH_QSEARCH, Move::NO_MOVE, TFlag::EXACT, static_cast<uint8_t>(pos.fullMoveNumber()));
        return stand_pat;
    }

    capture_gen.set_tt_move(transposition.move);
    
    if (depth == -QSEARCH_MAX_DEPTH || ss - stack >= SEARCH_STACK_SIZE - 1)
        return stand_pat;

    alpha = std::max(alpha, stand_pat);

    int max_value = stand_pat;

    Square previous_to_square = ((ss - 1)->current_move).to();

    while (capture_gen.next(move)){
        Piece captured_piece = pos.at(move.to());
        Piece moved_piece = pos.at(move.from());

        // delta pruning
        // move.score() is calculated with set_capture_score which is material difference.
        // 1500 is a safety margin
        if (move.typeOf() != Move::PROMOTION && move.to() != previous_to_square){
            if (stand_pat 
                + piece_value[static_cast<int>(captured_piece.type())]
                - piece_value[static_cast<int>(moved_piece.type())]
                + qs_fp_1 < alpha)
                continue;

            // SEE pruning
            if (!SEE::evaluate(pos, move, alpha - stand_pat - qs_see_1))
                continue;
    
            if (!pv && capture_gen.index() > 7
                && stand_pat + qs_p_1 < alpha && !SEE::evaluate(pos, move, -qs_p_2))
                continue;
        }

        ss->moved_piece = moved_piece;
        ss->current_move = move;
        pos.update_state(move);
        value = -qsearch<pv>(-beta, -alpha, depth-1, ss + 1);
        pos.restore_state(move);

        if (value > max_value){
            max_value = value;
            best_move = move;
        }

        alpha = std::max(alpha, value);
        if (alpha >= beta) // only check for cutoffs when alpha gets updated.
            break;
    }

    if (capture_gen.tt_move == Move::NO_MOVE && capture_gen.empty() && pos.try_outcome_eval(stand_pat)){
        transposition_table.store(zobrist_hash, stand_pat, NO_VALUE, DEPTH_QSEARCH,
            Move::NO_MOVE, TFlag::EXACT, static_cast<uint8_t>(pos.fullMoveNumber()));
        return stand_pat;
    }

    if (pos.halfMoveClock() + depth + QSEARCH_MAX_DEPTH >= 100)
        return max_value; // avoid storing history dependant evals.

    if (depth == 0)
        transposition_table.store(zobrist_hash, max_value,
            stand_pat,
            DEPTH_QSEARCH, best_move,
            max_value >= beta ? TFlag::LOWER_BOUND : TFlag::UPPER_BOUND,
            static_cast<uint8_t>(pos.fullMoveNumber()));

    if (is_mate(max_value))
        max_value = increment_mate_ply(max_value);

    return max_value;
}
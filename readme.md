# Overview
Bread engine is a chess engine written in c++. I started working on it in 2021, and only just finished. There is still a lot of room for improvement, but the engine is quite strong (for humans, at least). It uses NNUE (efficiently updatable neural network) to evaluate positions, as well as minimax search.
Bread engine does not have a GUI built in, however it supports the uci protocol, you can therefore run it on any chess GUI.

The engine has two build options:

- Option 1: use endgame tablebase
If this option is on, the engine will search for a folder next to the executable, named `3-4-5_pieces_Syzygy` containing the tablebase. You can download the folder [here](https://chess.massimilianogoi.com/download/tablebases/Syzygy3-4-5/). Thanks to Massimiliano Goi for sharing this tablebase. 

- Option 2: embed neural network in executable.
If this option is off, the engine will search for the folder `model_NNUE_2-40960_2x256_8_8_1_quant_layers` next to the executable.

# Installation
For windows, you can find precompiled binaries in the release section. There are 4 of them, one for each option combination.

If you want to change the tablebase or neural network path, you can build the engine yourself using cmake.

Once you have the engine installed, you can run the executable, and write the following commands:

```console
uci
position startpos
go wtime 10000 btime 10000 winc 1000 binc 1000
```
and the engine will return the best move. To set a position with a specific fen, you can use `position fen 5rkq/3prp1p/5RpP/p1p5/5Q2/1B4P1/P4PK1/8 w - a6 0 51` for instance. You can find more details on the [uci protocol](https://www.wbec-ridderkerk.nl/html/UCIProtocol.html) webpage. Please note that only the main commands are supported.

Besides the main engine build, you can also build some utility code to:
- generate the neural network header to be embeded in the executable
- tune some engine parameters
- run tests
- benchmark the engine
These can all be built using cmake.

# Technical details
in this section we provide an overview of the search algorithm, and the main engine features. The explanations are very high level, so feel free to search more details on other resources.

- [Search Algorithm](#search-algorithm)
  - [Minimax](#minimax)
  - [Alpha Beta pruning](#alpha-beta-pruning)
  - [Transposition Table](#transposition-table)
  - [Move ordering.](#move-ordering)
  - [Iterative deepening.](#iterative-deepening)
  - [Quiescence search](#quiescence-search)
  - [pondering](#pondering)
  - [aspiration windows.](#aspiration-windows)
  - [other optimizations](#other-optimizations)
  - [threefold repetition](#threefold-repetition)
- [Neural Network](#neural-network)
  - [Neural network types](#neural-network-types)
  - [Description of NNUE](#description-of-nnue)
  - [the halfKP feature set](#the-halfkp-feature-set)
  - [quantization](#quantization)

## Search Algorithm
A chess engine is only just a program that takes a chess position and returns the corresponding best move to play. To play a full game of chess, you can just give the engine the positions that arise on the board one after the other.

### Minimax
Quantifiying what a "good move" is in chess is tricky, it is easier to tell who has the advantage on a given position.

So let's say you wrote a function that takes a chess position and returns a score that is positive if white has the advantage, and negative if black has the advantage. A very basic implementation would be to count the white pieces, and subtract the number of black pieces.
The associated score is usually in centipawns, but the measure is arbitrary, the important is that the evaluation is coherent: if the position A is better than B for white, then f(A) > f(B).

Now that we have an evaluation function, we can use it to find moves: For a given chess position, we can look at every legal move, and choose the one that increases the evaluation function the most if it is white's turn to play, and decreases the evaluation the most if it is black's turn to play.
However, this is not enough to make a strong chess engine. The issue is that the evaluation function is never perfect, so the engine will make mistakes. If we use the "piece count" evaluation function, our engine may take a defended pawn with a queen, and loose it on the next move. 
Therefore, what we need to do is consider the opponent's responses to our moves. So when the engine thinks, after queen takes pawn, it will look at all the opponent's legal moves and evaluate these using the evaluation function. The opponent wants to minimize the score so it will pick the lowest evaluation. Now we can update the queen takes pawn position to be bad for us.

We saw how the engine can consider its move, and the opponent's response. We say that the engine searches at depth 2. But there is no reason to stop there, a depth of 3 means the engine considers: its move, the opponent's response, and its move again.
Usually, the deeper the engine can search, the better the output move will be. The main bottleneck is complexity: there is an average of 30 moves per chess position. So at depth 1, the engine needs to search 30 positions. At depth 2, it needs to search 30^2 = 900 positions, and in general 30^depth positions, which gets huge very quickly. The goal of the following sections is to optimize this algorithm.


### Alpha Beta pruning
In the minimax algorithm described earlier, there are some positions that we don't need to search for: if the engine has already found a move that guarantees +0.2 for white and while searching for the next move it finds a guaranteed -1.1 position for black, then there is no need to continue searching for black's other options, as white will never go down that path anyway.
Here is an explanation [video](https://www.youtube.com/watch?v=l-hh51ncgDI&t=562s).

### Transposition Table
In chess, it is possible to reach the same position with different moves. Therefore, to avoid searching for the same positions multiple times, we store hashes of the chess positions along with the score, the depth and best move in a transposition table, and use them when we encounter the position again. However, care must be taken when storing position evaluations. Indeed, with alpha beta pruning, some evaluations are only upper or lower bounds of the real evaluation.

### Move ordering.
To leverage the power of alpha beta pruning, it is best to "guess" what moves are best according to some heuristics, and sort the moves from best to worst. This way, more pruning will occur thanks to alpha beta pruning.

### Iterative deepening.
Iterative deepening consists of searching for positions at depth 1 first, then depth 2, depth 3 etc.
One might wonder why we need to search at depth 1 if we are going to search at depth 2 anyway. Isn't this a waste of time?
Actually, the results from the depth 1 search are stored in the transposition table, so even though they can't be reused because the depth is shallower, they can be used for enhanced move ordering. Counterintuitively, the benefit from better move ordering counteracts the time loss from searching at shallower depths. Therefore, to search at a given depth, it is better to search through each depth until the target depth is reached.
Moreover, iterative deepening takes care of the following problem: how to know ahead of time at which depth to search, for a limited time chess game? With iterative deepening, one can start a search at infinite depth, and interrupt it when the allocated time for the move has elapsed.

### Quiescence search
An issue with minimax is the horizon effect. We saw it in action when looking at the depth 1 example: the engine can see queen takes pawn, but not pawn takes queen on the next move. This issue was never resolved on leaf nodes. To mitigate positional misunderstanding by the AI, we can run a "quiescence search" instead of directly evaluating leaf nodes. The quiescence search is similar to minimax, but it only searches through captures (and checks) until the position becomes "quiet". At this point the evaluation function is more likely to give an accurate result.

### pondering
When the opponent is thinking, the engine can run a search on the move that it thinks is the best for the opponent. If the opponent plays the move, we call it a ponderhit, and the engine can play faster as it has already spent time searching for the position. If the opponent plays another move, the engine just searches for the position normally.

### aspiration windows.
After a deep enough search, we do not expect the evaluation to fluctuate a lot. At this point, we can use a "search window" on the root, and prune more branches that are outside the window. However, if the search falls low/high, we need to perform the search again.
This optimisation turned out to decrease the engine strength due to search instabilities and was therefore removed.

### other optimizations
the following optimizations are also implemented in bread engine:
- [late move reductions](https://www.chessprogramming.org/Late_Move_Reductions)
- [killer moves](https://www.chessprogramming.org/Killer_Move)
- [piece square tables for move ordering](https://www.chessprogramming.org/Piece-Square_Tables)
- [endgame tablebase](https://en.wikipedia.org/wiki/Endgame_tablebase)
- [futility pruning](https://www.chessprogramming.org/Futility_Pruning)


### threefold repetition
The first idea that comes to mind to implement threefold repetition is to have something like 
```c++
if (inner_board.isthreefoldrepetition()){
    return 0;
}
```
in the minimax function. However, care must be taken for the transposition table, as the evaluation of 0 is "history dependent". In another variation of the search, this position might be winning or losing, and reusing an evaluation of 0 would lead to engine blunders. Therefore, Bread Engine doesn't store positions that started repeating and were evaluated as 0. This way, the problematic positions aren't reused, but higher in the tree, the possibility of forcing a draw is taken into account.
Currently, the fifty-move rule isn't supported by Bread Engine.

## Neural Network
### Neural network types
In its first prototype, Bread Engine relied on a convolutional neural network as its evaluation function. You can find the network specs [here](./images/CNN%20specs.png?raw=true). Even though convolutional neural networks are suited for pattern recognition, they are too slow for a strong chess engine. A classic multilayer perceptron, on the other hand, may have weaker accuracy, but its speed more than makes up for it. Moreover, recent techniques make use of clever aspects of chess evaluation to speed up these networks even more.

### Description of NNUE
Bread Engine's NNUE is heavily inspired by the approach described by [stockfish](https://github.com/official-stockfish/nnue-pytorch/blob/master/docs/nnue.md).

When evaluating leaf nodes during minimax search, one can note that the positions encountered are very similar. Therefore, instead of running the whole neural network from scratch, the first layer output is cached and reused.

Let's consider a simple deep neural network, taking 768 inputs corresponding to each piece type on each possible square (so 12 * 64 in total). Suppose we have already run the neural network on the starting position, and that we cached the first layer output before applying the activation function (this is called the "accumulator"). If we move a pawn from e2 to e4, we effectively turn off the input neuron number n corresponding to "white pawn on e2" and turn on the input neuron m corresponding to "white pawn on e4". Recall that computing a dense layer output before applying bias and activation is just matrix multiplication. Therefore to update the accumulator, we need to subtract the n-th row of weights from the accumulator, and add the m-th row of weights. The effect of changing the two input neurons on further layers is non trivial, and these need to be recomputed. Therefore, it is advantageous to have a large input layer, and small hidden layers.

### the halfKP feature set
There are many different choices for the input feature representation, but 
> HalfKP is the most common feature set and other successful ones build on top of it. It fits in a sweet spot of being just the right size, and requiring very few updates per move on average. [Link](https://github.com/official-stockfish/nnue-pytorch/blob/master/docs/nnue.md#halfkp)

This board representation works with two components: the perspective of the side to move, and the other perspective.
Each perspective consists of a board representation similar to before, but with separate features for each friendly king square.
So there are 64 king squares * 64 piece squares * 10 piece types (not including the kings) = 40960 input features for one perspective.
This overparametrisation from the intuitive 768 to 40960 input neurons helps to leverage the efficiency of NNUE.
In the case of Bread Engine, the two perspectives are each run through the same set of weights 40960->256 and concatenated into a vector of size 512, with the side to move perspective first, and the other second.
Here are the full [network specs](./images/NNUE%20specs.png?raw=true) (Relu is actually clipped relu between 0 and 1).

### quantization
An important way to speed up the neural network is to use quantization with SIMD vectorization on the cpu (unfortunately, gpu's aren't usually suited for chess engines as minimax is difficult to paralellize because of alpha beta pruning (see [ybwf](https://www.chessprogramming.org/Young_Brothers_Wait_Concept) or [lazy SMP](https://www.chessprogramming.org/Lazy_SMP)). Also, the data transfer latency between cpu and gpu is too high).

The first layer weights and biases are multiplied by a scale of 127 and stored in int16. Accumulation also happens in int16.  With a maximum of 30 active input features (all pieces on the board), there won't be any integer overflow unless (sum of 30 weights) + bias > 32767, which is most definitely not the case. We then apply clipped relu while converting to int8. Now layer_1_quant_output = input @ (127*weights) + 127*bias = 127 * (input @ weights + bias) = 127 * true_output.
For the second layer, weights are multiplied by 64 and stored in int8, and bias is multiplied by 127 * 64 and stored in int32.
To be able to store weights in int8, we need to make sure that weights*64 < 127, so a weight can't exceed 127/64 = 1.98. This is the price to pay for full integer quantization. After this layer, the output is
layer_2_quant_output = quant_input @ (64*weights) + 127*64*bias = 127*64*(input @ weights + bias) = 127*64*true_output. Therefore, we need to divide the quantized output by 64 before applying the clipped relu (this is done using bitshifts as 64 is a power of 2. We lose at most 1/127 = 0.0078 of precision by doing this).
Layers 3 and 4 are similar. Layer 4 is small enough that we can accumulate outputs in in16.
Here is the relevant code in Bread Engine:

```c++
inline __m256i _mm256_add8x256_epi32(__m256i* inputs){ // horizontal add 8 int32 avx registers.
    inputs[0] = _mm256_hadd_epi32(inputs[0], inputs[1]);
    inputs[2] = _mm256_hadd_epi32(inputs[2], inputs[3]);
    inputs[4] = _mm256_hadd_epi32(inputs[4], inputs[5]);
    inputs[6] = _mm256_hadd_epi32(inputs[6], inputs[7]);

    inputs[0] = _mm256_hadd_epi32(inputs[0], inputs[2]);
    inputs[4] = _mm256_hadd_epi32(inputs[4], inputs[6]);

    return _mm256_add_epi32(
        // swap lanes of the two regs
        _mm256_permute2x128_si256(inputs[0], inputs[4], 0b00110001),
        _mm256_permute2x128_si256(inputs[0], inputs[4], 0b00100000)
    );
};

template<int in_size, int out_size>
void HiddenLayer<in_size, out_size>::run(int8_t* input, int32_t* output){

    __m256i output_chunks[int32_per_reg]; // these will be accumulated in ont register later
    const __m256i one = _mm256_set1_epi16(1);

    for (int j = 0; j < num_output_chunks; j++){
        __m256i result = _mm256_set1_epi32(0);
        for (int i = 0; i < num_input_chunks; i++){
            __m256i input_chunk = _mm256_loadu_epi8(&input[i*int8_per_reg]);
            for (int k = 0; k < int32_per_reg; k++){
                output_chunks[k] = _mm256_maddubs_epi16(
                    input_chunk,
                    _mm256_loadu_epi8(&this->weights[(j*int32_per_reg+k) * this->input_size + i*int8_per_reg])
                );
                // j*output_chunk_size+k is the j*8+k-th row, to that offset horizontally by i*32.
                output_chunks[k] = _mm256_madd_epi16(output_chunks[k], one); // hadd pairs to int32
            }
            result = _mm256_add_epi32(result, _mm256_add8x256_epi32(output_chunks));
        }
        result = _mm256_add_epi32(result, _mm256_loadu_epi32(&this->bias[j*int32_per_reg]));
        result = _mm256_srai_epi32(result, 6); // this integer divides the result by 64 to scale back the output.
        _mm256_storeu_epi32(&output[j*int32_per_reg], result);
    }
};
```

# Learning resources
[chess programming wiki](https://www.chessprogramming.org/Main_Page)
[wikipedia](https://en.wikipedia.org/wiki/Monte_Carlo_tree_search)
[avx tutorial](https://www.codeproject.com/Articles/874396/Crunching-Numbers-with-AVX-and-AVX)
[intel avx](https://www.intel.com/content/www/us/en/docs/intrinsics-guide/index.html#ig_expand=140,92,83)
- posts:
[nnue](https://chess.stackexchange.com/questions/33691/halfkp-structure-for-stockfish-shogi-nnue-how-does-it-work)

# Notable games
nnue without quantization vs nnue with quantization 0-1: 
1. d4 Nf6 2. c4 c5 3. d5 e6 4. Nc3 exd5 5. cxd5 d6 6. e4 g6 7. Bf4 a6 8. Bd3 b5
9. Nge2 Bg7 10. a3 O-O 11. O-O Nh5 12. Bc1 Nd7 13. h3 Ne5 14. Bc2 Nf3+ 15. gxf3
Bxh3 16. Ng3 Qh4 17. Nce2 Rae8 18. Re1 Bd4 19. Nxd4 Nxg3 20. Ne2 Nxe2+ 21. Qxe2
f5 22. Bd2 f4 23. Bxf4 Rxf4 24. Kh2 Bf5+ 25. Kg2 Qg5+ 26. Kf1 Bh3# 0-1

# Acknowledgements
[lichess's open database](https://database.lichess.org/) for training data for the neural network.
[stockfish](https://stockfishchess.org/) for their documentation and training evaluations.
[tensorflow](https://www.tensorflow.org/)
[onnx runtime](https://onnxruntime.ai/)
[sebastian lague](https://www.youtube.com/watch?v=U4ogK0MIzqk&t=1174s) for inspiring me to do this project
[disservin's chess library](https://github.com/Disservin/chess-library)
[fathom tablebase probe](https://github.com/jdart1/Fathom)

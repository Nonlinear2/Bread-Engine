<h1 align="center">Bread Engine</h1>
<h4 align="center">A superhuman chess engine written in C++.</h4>

<p align="center">
  <a>
    <img src="https://img.shields.io/badge/Architectures-x86%2C%20x64-%23f78b04?style=for-the-badge&labelColor=%23013c5a&color=%23013c5a">
  </a>
  <a href="https://github.com/Nonlinear2/Bread-Engine/releases">
    <img src="https://img.shields.io/github/v/release/Nonlinear2/Bread-Engine?include_prereleases&style=for-the-badge&label=Lastest%20Release&labelColor=%23950502&color=%23013c5a">
  </a>
  <a>
    <img src="https://img.shields.io/github/license/Nonlinear2/Bread-Engine?style=for-the-badge&labelColor=%23950502&color=%23950502">
  </a>
</p>

# Overview
Bread is rated around 3000 elo on the computer chess rating lists.

It uses minimax search, along with an efficiently updatable neural network trained from zero knowledge. The neural network is homemade and trained using games of self play.

Bread engine does not have a graphical interface built in. However it supports the UCI protocol, you can therefore run it on any chess graphical interface, such as <a href="https://encroissant.org/"><kbd>En Croissant</kbd></a> or <a href="https://github.com/cutechess/cutechess"><kbd>Cute Chess</kbd></a>.

# Installation

<table>
<tr><td bgcolor="#fff3cd">
  
**Please note:** The engine requires a CPU with **AVX2** support.

</td></tr>
</table>

You can download precompiled binaries for Windows in the <a href="https://github.com/Nonlinear2/Bread-Engine/releases"><kbd>Release Section</kbd></a>.

To use the engine on Linux, you need to build the project yourself.

## How to build the project yourself

You need Cmake and a C++ compiler installed. The build has been tested using compilers Clang 20.1.7 and GCC 13.1.0.

### Windows


Clone the repository, open a command line interface, navigate to the project's root directory and type:
```powershell
mkdir build
cd build
cmake ..
cmake --build . --target release --config release
```
This will generate a build folder with the executable.

## Linux

Run the script `compile_script_for_linux.sh`. The executable will be generated in the `build` folder.

---

If you want to check whether everything is working properly, you can run the executable and write the following commands one after the other:

```console
uci
position startpos
go movetime 3000
```

and the engine will return the best move.

# Custom commands

Bread engine supports the following commands:
- `bench`: gives the total number of nodes and nodes per seconds on a series of positions.
- `bench nn`: gives the speed of the neural network inference

Bread engine also has a uci option called `nonsense`, which makes it bongcloud, underpromote to bishops and knights, as well as print lyrics from "never gonna give you up" during search.

# Technical details
in this section we provide an overview of the search algorithm, and the main engine features. The explanations are very high level, so feel free to search more details on other resources.

- [Search Algorithm](#search-algorithm)
  - [Minimax](#minimax)
  - [Alpha Beta pruning](#alpha-beta-pruning)
  - [Transposition Table](#transposition-table)
  - [Move ordering](#move-ordering)
  - [Iterative deepening](#iterative-deepening)
  - [Quiescence search](#quiescence-search)
  - [pondering](#pondering)
  - [other optimizations](#other-optimizations)
  - [threefold repetition](#threefold-repetition)
- [Neural Network](#neural-network)
  - [Neural network types](#neural-network-types)
  - [Description of NNUE](#description-of-nnue)
  - [Feature sets](#feature-sets)
  - [quantization](#quantization)
  - [optimized matrix multiplication](#optimized-matrix-multiplication-not-currently-in-use)

## Search Algorithm
A chess engine is just a program that takes a chess position and returns the corresponding best move to play. To play a full game of chess, you can give the engine the positions that arise on the board one after the other.

### Minimax
A "good move" in chess can be defined as a move which "improves your position".

So let's say you wrote a function that takes a chess position and returns a score that is positive if white has the advantage, and negative if black has the advantage. A very basic implementation would be to count the white pieces, and subtract the number of black pieces.
The associated score is usually in centipawns but the measure is arbitrary, what matters is that the evaluation is coherent: if the position A is better than B for white, then f(A) > f(B).

Now that we have an "evaluation function", we can use it to find moves: For a given chess position, we can look at every legal move, and choose the one that increases the evaluation function the most if it is white's turn to play, and decreases the evaluation the most if it is black's turn to play.
However, this is not enough to make a strong chess engine. The issue is that the evaluation function is never perfect, so the engine will make mistakes. If we use the "piece count" evaluation function, our engine may take a defended pawn with a queen, and loose it on the next move. 
Therefore, what we need to do is consider the opponent's responses to our moves. So when the engine thinks, after queen takes pawn, it will look at all the opponent's legal moves and evaluate these using the evaluation function. The opponent wants to minimize the score so it will pick the lowest evaluation. Now we can update the queen takes pawn position to be bad for us.

We saw how the engine can consider its move, and the opponent's response. We say that the engine searches at depth 2. But there is no reason to stop there, a depth of 3 means the engine considers: its move, the opponent's response, and its move again.
Usually, the deeper the engine can search, the better the output move will be. The main bottleneck is complexity: there is an average of 30 moves per chess position. So at depth 1, the engine needs to search 30 positions. At depth 2, it needs to search 30^2 = 900 positions, and in general 30^depth positions, which gets huge very quickly. The goal of the following sections is to optimize this algorithm with different kinds of improvements. Some of them are speedups and don't change the output of the engine whereas others do, but reduce the number of positions searched to find a good move. Note that with these improvements, depth becomes less meaningful of a metric, as parts of the search tree get cut and others get extended. Knowing that a chess engine has finished searching at a given depth only gives a vague idea of how much of the search tree has been explored.


### Alpha Beta pruning
In the minimax algorithm described previously, we can avoid searching certain moves: if the engine has already found a move that guarantees +0.2 for white and while searching for the next move it finds a guaranteed -1.1 position for black, then there is no need to continue searching for black's other options, as white will never go down that path anyway.
This [video](https://www.youtube.com/watch?v=l-hh51ncgDI&t=562s) gives a visual explanation of alpha beta pruning.

### Transposition Table
In chess, it is possible to reach the same position with different moves. Therefore to avoid searching for the same positions multiple times, we can store hashes of chess positions we encounter, along with the score and some additional information, and use the stored data if we encounter the position again. However care must be taken when storing position evaluations as with alpha beta pruning some evaluations are only upper or lower bounds of the real evaluation.

### Move ordering
To leverage the power of alpha beta pruning, it is best to "guess" what moves are best according to some heuristics, and sort the moves from best to worst. This way, more pruning will occur thanks to alpha beta pruning.

### Iterative deepening
Iterative deepening consists of searching for positions at depth 1 first, then depth 2, depth 3 etc.
The reason it is preferable to search at reduced depths before higher depths is to populate the transposition table, as well as "histories" which can speedup greatly deeper searches by enhancing move ordering, for instance. Counterintuitively, these benefits counteract the time loss from searching at shallower depths. Therefore, to search at a given depth, it is better to search through each depth until the target depth is reached.
Moreover, using iterative deepening means we don't need to know ahead of time at which depth to search for a limited time chess game. One can simply start a search at infinite depth, and interrupt it when the allocated time for the move has elapsed.

### Quiescence search
An issue with minimax is the horizon effect. We saw it in action when looking at the depth 1 example: the engine can see queen takes pawn, but not pawn takes queen on the next move. Even at higher depth, this issue persists on leaf nodes which can lead to misevaluations. To mitigate positional misunderstanding by the AI, we can run a "quiescence search" instead of directly evaluating leaf nodes. Quiescence search is similar to minimax, but it only searches through captures (and checks) until the position becomes "quiet". At this point the evaluation function is more likely to give an accurate result.

### Pondering
When the opponent is thinking, the engine can search the move that it thinks is the best for the opponent. If the opponent does play the move, the engine can play faster as it has already spent time searching for the position. If the opponent plays another move, the engine just searches for the position normally.

### Other optimizations
Many other search improvements exist, but they would be too long to describe in this readme. Here is a list containing a few of them which are implemented in Bread: 
- late move reductions
- killer moves
- piece square tables for move ordering
- endgame tablebases
- futility pruning
- history pruning
- singular extensions
- reverse futility pruning
- razoring
- continuation history

  ...


### Threefold repetition
The first idea that comes to mind to implement threefold repetition is to have something like 
```c++
if (pos.is_threefold_repetition()){
    return 0;
}
```
in the minimax function. However, care must be taken for the transposition table, as the evaluation of 0 is "history dependent". In another variation of the search, this position might be winning or losing, and reusing an evaluation of 0 would lead to engine blunders. Therefore, Bread Engine doesn't store positions that started repeating and were evaluated as 0. This way, the problematic positions aren't reused, but higher in the tree, the possibility of forcing a draw is taken into account.

## Neural Network
### Neural network types
In its first prototype, Bread Engine relied on a convolutional neural network as its evaluation function. You can find the network specs [here](./images/CNN%20specs.png?raw=true). Even though convolutional neural networks are suited for pattern recognition, they are too slow for a strong chess engine. A classic multilayer perceptron, on the other hand, may have weaker accuracy, but its speed more than makes up for it. Moreover, recent techniques make use of clever aspects of chess evaluation to speed up these networks even more.

### Description of NNUE
When evaluating leaf nodes during minimax search, one can note that the positions encountered are very similar. Therefore, instead of running the whole neural network from scratch, the first layer output is cached and reused.

Let's consider a simple deep neural network, taking 768 inputs corresponding to each piece type on each possible square (so 12 * 64 in total). Suppose we have already run the neural network on the starting position, and that we cached the first layer output before applying the activation function (this is called the "accumulator"). If we move a pawn from e2 to e4, we turn off the input neuron number n corresponding to "white pawn on e2" and turn on the input neuron m corresponding to "white pawn on e4". Recall that computing a dense layer output before applying bias and activation is just matrix multiplication. Therefore to update the accumulator, we need to subtract the n-th row of weights from the accumulator, and add the m-th row of weights. The effect of changing the two input neurons on further layers is non trivial, and these need to be recomputed. Therefore, it is advantageous to have a large input layer, and small hidden layers.

### Feature sets
There are many different choices for the input feature representation, and the choice mainly depends on how much training data is available. 
For now, Bread uses a 768 input feature set described by (color, square, piece) for the perspective of the side to move, and the other perspective.
The two perspectives are each run through the same set of weights and concatenated into a single vector, with the side to move perspective first, and the other second. This effectively encodes the side to move in the network evaluation.

### Quantization
An important way to speed up the neural network is to use quantization with SIMD vectorization on the cpu (unfortunately, gpu's aren't usually suited for chess engines as minimax is difficult to paralellize because of alpha beta pruning (for a multithreading approach for chess engines, see [lazy SMP](https://www.chessprogramming.org/Lazy_SMP)). Also, the data transfer latency between cpu and gpu is too high).

The first layer weights and biases are multiplied by a scale of 255 and stored in int16. Accumulation also happens in int16.  With a maximum of 30 active input features (all pieces on the board), there won't be any integer overflow unless (sum of 30 weights) + bias > 32767, which is most definitely not the case. We then apply clipped relu while converting to int8. Now `layer_1_quant_output = input @ (255*weights) + 255*bias = 255*(input @ weights + bias) = 255*true_output`.
For the second layer, weights are multiplied by 64 and stored in int8, and the bias is multiplied by 127 * 64 and stored in int32.
To be able to store weights in int8, we need to make sure that weights * 64 < 127, so a weight can't exceed 127/64 = 1.98. This is the price to pay for full integer quantization. After this layer, the output is
`layer_2_quant_output = quant_input @ (64*weights) + 255*64*bias = 255*64*(input @ weights + bias) = 255*64*true_output`. We can finally divide this by a given scale to get the network output.

Here is some code to run the hidden layers (this was used until Bread Engine 0.0.6):
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

    __m256i output_chunks[int32_per_reg];
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

### Optimized matrix multiplication (not currently in use)
*The following method is original, it may or may not be a good approach.*

To optimize the matrix multiplication code above, one can note that in the `_mm256_add8x256_epi32` function, we use `_mm256_hadd_epi32` for horizontal accumulation. However, we would like to use `_mm256_add_epi32` which is faster, but does vertical accumulation. To achieve this we can shuffle vertically the weights in the neural network, to have the weights that need to be added together on different rows. After the multiplication, we can shuffle the weights horizontally so they align, and add them vertically.

```c++
inline __m256i _mm256_add8x256_epi32(__m256i* inputs){

    inputs[1] = _mm256_castps_si256(_mm256_permute_ps(_mm256_castsi256_ps(inputs[1]), 0b00111001));
    inputs[2] = _mm256_castps_si256(_mm256_permute_ps(_mm256_castsi256_ps(inputs[2]), 0b01001110));
    inputs[3] = _mm256_castps_si256(_mm256_permute_ps(_mm256_castsi256_ps(inputs[3]), 0b10010011));

    inputs[5] = _mm256_castps_si256(_mm256_permute_ps(_mm256_castsi256_ps(inputs[5]), 0b00111001));
    inputs[6] = _mm256_castps_si256(_mm256_permute_ps(_mm256_castsi256_ps(inputs[6]), 0b01001110));
    inputs[7] = _mm256_castps_si256(_mm256_permute_ps(_mm256_castsi256_ps(inputs[7]), 0b10010011));

    inputs[0] = _mm256_add_epi32(inputs[0], inputs[1]);
    inputs[2] = _mm256_add_epi32(inputs[2], inputs[3]);
    inputs[0] = _mm256_add_epi32(inputs[0], inputs[2]);

    
    inputs[4] = _mm256_add_epi32(inputs[4], inputs[5]);
    inputs[6] = _mm256_add_epi32(inputs[6], inputs[7]);
    inputs[4] = _mm256_add_epi32(inputs[4], inputs[6]);
    
    return _mm256_add_epi32(
        inputs[0],
        // swap lanes of the second register
        _mm256_castps_si256(_mm256_permute2f128_ps(_mm256_castsi256_ps(inputs[4]), _mm256_castsi256_ps(inputs[4]), 1))
    );
};
```
In the following representation, same digits need to be added together.

- Previous `_mm256_add8x256_epi32` function:

| input 1: | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 |
|----------|---|---|---|---|---|---|---|---|
| input 2: | 1 | 1 | 1 | 1 | 1 | 1 | 1 | 1 |
| input 3: | 2 | 2 | 2 | 2 | 2 | 2 | 2 | 2 | 
| input 4: | 3 | 3 | 3 | 3 | 3 | 3 | 3 | 3 |
| input 5: | 4 | 4 | 4 | 4 | 4 | 4 | 4 | 4 |
| input 6: | 5 | 5 | 5 | 5 | 5 | 5 | 5 | 5 |
| input 7: | 6 | 6 | 6 | 6 | 6 | 6 | 6 | 6 |
| input 8: | 7 | 7 | 7 | 7 | 7 | 7 | 7 | 7 |

| output register: | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 |
|------------------|---|---|---|---|---|---|---|---|

- New `_mm256_add8x256_epi32` function (takes vertically shuffled input):

| input 0: | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 |
|----------|---|---|---|---|---|---|---|---|
| input 1: | 3 | 0 | 1 | 2 | 7 | 4 | 5 | 6 |
| input 2: | 2 | 3 | 0 | 1 | 6 | 7 | 4 | 5 |
| input 3: | 1 | 2 | 3 | 0 | 5 | 6 | 7 | 4 |
| input 4: | 4 | 5 | 6 | 7 | 0 | 1 | 2 | 3 |
| input 5: | 7 | 4 | 5 | 6 | 3 | 0 | 1 | 2 |
| input 6: | 6 | 7 | 4 | 5 | 2 | 3 | 0 | 1 |
| input 7: | 5 | 6 | 7 | 4 | 1 | 2 | 3 | 0 |

intermediate result using `_mm256_permute_ps`:

| register 0: | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 |
|-------------|---|---|---|---|---|---|---|---|
| register 1: | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 |
| register 2: | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 |
| register 3: | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 |
| register 4: | 4 | 5 | 6 | 7 | 0 | 1 | 2 | 3 |
| register 5: | 4 | 5 | 6 | 7 | 0 | 1 | 2 | 3 |
| register 6: | 4 | 5 | 6 | 7 | 0 | 1 | 2 | 3 |
| register 7: | 4 | 5 | 6 | 7 | 0 | 1 | 2 | 3 |

vertical sum of the 4 first and last registers:

| register 0: | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 |
|-------------|---|---|---|---|---|---|---|---|
| register 1: | 4 | 5 | 6 | 7 | 0 | 1 | 2 | 3 |

swap lanes of the second register and vertical sum:

| output register: | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 |
|------------------|---|---|---|---|---|---|---|---|


# Learning resources
- [chess programming wiki](https://www.chessprogramming.org/Main_Page)

- [avx tutorial](https://www.codeproject.com/Articles/874396/Crunching-Numbers-with-AVX-and-AVX)

- [intel intrinsics guide](https://www.intel.com/content/www/us/en/docs/intrinsics-guide/index.html#ig_expand=140,92,83)

# Notable games
- Bread Engine 0.0.9 vs chess.com's Magnus Carlsen bot 1-0:

```
1. d4 Nf6 2. Nf3 d5 3. c4 e6 4. Nc3 Be7 5. cxd5 exd5 6. Bf4 O-O 7. e3 Nh5 8. Be5
Nc6 9. h3 Be6 10. Bh2 Nf6 11. Bd3 Nb4 12. Bb1 c5 13. dxc5 Bxc5 14. O-O Nc6 15. Bc2
Rc8 16. Rc1 a6 17. Bg3 Ba7 18. Qe2 d4 19. Rfd1 Re8 20. Bh4 b5 21. Ne4 Bc4 22.
Qe1 Bxa2 23. b3 Bb6 24. Qe2 Rxe4 25. Bxe4 Bxb3 26. Bxh7+ Kh8 27. Bc2 Bc4 28. Bd3 Bb3
29. Bc2 Bc4 30. Bd3 Bb3 31. Bf5 Bxd1 32. Qxd1 Rc7 33. Bg3 Ne7 34. Bxc7 Bxc7 35.
Qxd4 Kg8 36. Qa7 Ne8 37. Bc2 a5 38. Qc5 Nd6 39. Rd1 Qd7 40. Nd4 Nec8 41. Qh5 g6 42.
Bxg6 fxg6 43. Qxg6+ Kf8 44. e4 b4 45. Rd3 Qe8 46. Qh6+ Ke7 47. e5 Bb6 48. Nf5+
Kd8 49. Nxd6 Bxf2+ 50. Kxf2 Nxd6 51. Qxd6+ Kc8 52. Qa6+ Kb8 53. Rd6 Qf7+ 54. Kg1 Qc7
55. Rb6+ Qxb6+ 56. Qxb6+ Kc8 57. Qxa5 b3 58. Qc3+ Kd7 59. Qxb3 Ke8 60. Qe6+ Kf8 61.
Qf6+ Ke8 62. h4 Kd7 63. e6+ Kd6 64. e7+ Kd7 65. Qf8 Kc6 66. e8=Q+ Kd5 67. Qf5+
Kd4 68. Qee4+ Kc3 69. Qc5+ Kb2 70. Qec2+ Ka1 71. Qa3#
```

- Bread Engine 1.6.0 vs Simple Eval 1-0:

```
1. e4 e6 2. d4 d5 3. Nc3 Nf6 4. e5 Nfd7 5. f4 c5 6. Nf3 Nc6 7. Be3 cxd4 8. Nxd4
Qb6 9. Qd2 Nxd4 10. Bxd4 Bc5 11. Na4 Qc6 12. Nxc5 Nxc5 13. Qa5 Nd7 14. O-O-O a6
15. Qa3 Nb8 16. Rd3 Qc7 17. Bc5 f6 18. Be2 Qf7 19. Re1 fxe5 20. fxe5 Qf4+ 21.
Kb1 Nc6 22. Rh3 g6 23. Bd3 Qg5 24. Rf1 d4 25. Rxh7 Rxh7 26. Bxg6+ Kd7 27. Bxd4
Nxe5 28. Bxh7 a5 29. Be4 Qe7 30. Qxe7+ Kxe7 31. Bxe5 a4 32. Bf6+ Kd6 33. Rd1+
Kc5 34. Rf1 Kb5 35. Bd4 Bd7 36. Rf4 Bc6 37. Bd3+ Ka5 38. Bc3+ Kb6 39. g4 Rg8 40.
h4 Re8 41. Bd4+ Ka5 42. Bc3+ Kb6 43. Bd4+ Ka5 44. b4+ axb3 45. Bc3+ Kb6 46. Bd4+
Kc7 47. Be5+ Kb6 48. axb3 Kc5 49. Bd4+ Kd5 50. c4+ Kd6 51. Bc3 e5 52. Rf6+ Ke7
53. Rf2 e4 54. Be2 Kd8 55. Kb2 e3 56. Rf6 Ke7 57. g5 Rg8 58. b4 Kd8 59. Re6 Re8
60. Bf6+ Kd7 61. Rxe8 Kxe8 62. Kc3 Be4 63. Kd4 Bc2 64. Kxe3 Kf7 65. c5 Ke6 66.
Bf3 Kd7 67. Kd2 Bf5 68. Ke3 Bg6 69. Be5 Ke6 70. Bf6 Kd7 71. c6+ bxc6 72. Bg4+
Kd6 73. h5 Bb1 74. Bd4 Bc2 75. Bc5+ Kd5 76. Bf3+ Ke5 77. Bd4+ Kf5 78. Bf6 Bb3
79. h6 Bg8 80. Bxc6 Kg4 81. g6 Kf5 82. h7 Bxh7 83. gxh7 Kxf6 84. b5 Ke7 85. b6
Kd6 86. b7 Kc5 87. b8=Q Kxc6 88. h8=Q Kd5 89. Qb5+ Ke6 90. Qd7+ Kxd7 91. Qf6 Kc7
92. Kd4 Kc8 93. Kc5 Kd7 94. Qf7+ Kd8 95. Kb6 Kc8 96. Qc7# 1-0
```

# Acknowledgements

- [stockfish](https://stockfishchess.org/) for their amazing community.

- [bullet](https://github.com/jw1912/bullet) 

- [sebastian lague](https://www.youtube.com/watch?v=U4ogK0MIzqk&t=1174s) for inspiring me to do this project

- [disservin's chess library](https://github.com/Disservin/chess-library)

- [fathom tablebase probe](https://github.com/jdart1/Fathom)
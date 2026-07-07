#pragma once

#include <assert.h>
#include <cstdint>
#include <cmath>
#include <array>

/*************
NNUE constants
*************/

constexpr int NUM_INPUT_BUCKETS = 8;
constexpr int INPUT_BUCKETS[64] = {
    0, 1, 2, 3,  3, 2, 1, 0,
    4, 4, 5, 5,  5, 5, 4, 4,
    6, 6, 6, 6,  6, 6, 6, 6,
    6, 6, 6, 6,  6, 6, 6, 6,
    7, 7, 7, 7,  7, 7, 7, 7,
    7, 7, 7, 7,  7, 7, 7, 7,
    7, 7, 7, 7,  7, 7, 7, 7,
    7, 7, 7, 7,  7, 7, 7, 7,
};

constexpr int NUM_OUTPUT_BUCKETS = 8;

constexpr int INPUT_SIZE = 768 * NUM_INPUT_BUCKETS;
constexpr int ACC_SIZE = 1536;

constexpr int L1_INPUT_SIZE = ACC_SIZE;
constexpr int L1_OUTPUT_SIZE = 16;

constexpr int L2_INPUT_SIZE = L1_OUTPUT_SIZE * 2; // * 2 comes from dual activation
constexpr int L2_OUTPUT_SIZE = 1;


constexpr int L0_WEIGHTS_SIZE = INPUT_SIZE * ACC_SIZE;
constexpr int L0_BIAS_SIZE = ACC_SIZE;

constexpr int L1_WEIGHTS_SIZE = L1_INPUT_SIZE * L1_OUTPUT_SIZE;
constexpr int L1_BIAS_SIZE = L1_OUTPUT_SIZE;

constexpr int L2_WEIGHTS_SIZE = L2_INPUT_SIZE * L2_OUTPUT_SIZE;
constexpr int L2_BIAS_SIZE = L2_OUTPUT_SIZE;

constexpr int BUCKETED_L1_WEIGHTS_SIZE = NUM_OUTPUT_BUCKETS * L1_WEIGHTS_SIZE;
constexpr int BUCKETED_L1_BIAS_SIZE = NUM_OUTPUT_BUCKETS * L1_BIAS_SIZE;

constexpr int BUCKETED_L2_WEIGHTS_SIZE = NUM_OUTPUT_BUCKETS * L2_WEIGHTS_SIZE;
constexpr int BUCKETED_L2_BIAS_SIZE = NUM_OUTPUT_BUCKETS * L2_BIAS_SIZE;

using Accumulator = std::array<int16_t, ACC_SIZE>;
using Accumulators = std::array<Accumulator, 2>;
using ClampedAccumulators = std::array<std::array<int8_t, ACC_SIZE>, 2>;

constexpr int FT_PERMUTATION[ACC_SIZE / 2] = {
    519, 197, 63, 158, 546, 713, 255, 496, 32, 50, 23, 328, 362, 441, 623, 136, 183, 447, 592, 628, 581, 687, 210, 582, 54, 258, 357, 358, 646, 44, 205, 326, 693, 219, 252, 295, 685, 734, 170, 213, 402, 585, 72, 80, 116, 552, 67, 399, 560, 40, 188, 364, 293, 341, 520, 587, 118, 196, 254, 394, 418, 56, 76, 139, 460, 641, 650, 712, 151, 695, 730, 755, 25, 66, 311, 366, 639, 182, 281, 436, 681, 729, 126, 185, 419, 521, 527, 606, 638, 707, 452, 349, 380, 476, 684, 241, 459, 637, 109, 246, 314, 385, 508, 515, 348, 503, 108, 140, 283, 294, 573, 670, 161, 178, 302, 497, 647, 653, 683, 340, 506, 663, 31, 101, 410, 424, 449, 485, 578, 754, 156, 166, 446, 535, 605, 13, 312, 372, 193, 324, 350, 680, 18, 230, 412, 551, 667, 124, 296, 338, 728, 68, 106, 111, 207, 682, 141, 235, 492, 390, 498, 511, 715, 731, 62, 483, 758, 38, 209, 559, 28, 216, 479, 576, 616, 58, 45, 456, 742, 9, 91, 217, 256, 486, 547, 704, 225, 234, 409, 450, 467, 727, 499, 571, 610, 423, 525, 563, 186, 593, 12, 222, 433, 620, 87, 119, 190, 356, 471, 583, 1, 321, 427, 502, 691, 733, 236, 260, 396, 36, 171, 455, 482, 542, 565, 738, 147, 279, 323, 669, 145, 330, 724, 513, 622, 282, 290, 22, 43, 403, 428, 524, 575, 228, 540, 60, 128, 365, 688, 94, 327, 591, 604, 287, 443, 500, 600, 726, 764, 16, 478, 73, 104, 611, 359, 400, 756, 84, 322, 465, 411, 599, 661, 107, 203, 444, 594, 659, 743, 99, 150, 263, 159, 522, 642, 277, 325, 361, 191, 242, 250, 425, 435, 550, 574, 634, 655, 265, 376, 125, 226, 333, 584, 721, 240, 517, 763, 181, 261, 267, 278, 530, 30, 487, 636, 300, 675, 389, 566, 165, 198, 275, 429, 656, 673, 674, 53, 79, 167, 523, 589, 635, 223, 233, 274, 352, 518, 626, 89, 202, 512, 220, 601, 469, 29, 81, 138, 351, 766, 122, 206, 253, 440, 718, 74, 630, 748, 10, 21, 231, 438, 697, 725, 558, 618, 652, 129, 699, 3, 7, 187, 286, 595, 52, 432, 537, 86, 148, 373, 532, 598, 654, 61, 397, 760, 218, 739, 744, 301, 627, 180, 751, 405, 475, 484, 208, 257, 439, 609, 759, 319, 270, 175, 370, 177, 375, 382, 176, 304, 510, 602, 677, 344, 117, 123, 251, 719, 82, 526, 545, 259, 337, 481, 464, 678, 377, 384, 762, 318, 672, 723, 308, 199, 98, 633, 696, 8, 41, 347, 115, 417, 603, 705, 711, 24, 554, 720, 121, 437, 245, 702, 19, 354, 331, 378, 714, 671, 706, 285, 46, 103, 221, 448, 303, 65, 33, 289, 297, 305, 192, 307, 69, 280, 451, 514, 445, 461, 507, 590, 493, 137, 332, 335, 248, 619, 42, 144, 371, 676, 608, 0, 11, 509, 586, 110, 247, 422, 457, 168, 388, 640, 698, 83, 470, 569, 735, 134, 607, 49, 85, 490, 239, 135, 244, 717, 745, 543, 549, 153, 415, 426, 534, 454, 27, 572, 174, 504, 93, 462, 612, 401, 528, 629, 501, 155, 229, 298, 488, 430, 442, 90, 624, 473, 689, 690, 160, 194, 387, 201, 6, 189, 614, 658, 266, 284, 249, 353, 406, 120, 374, 458, 644, 163, 237, 420, 588, 737, 710, 152, 708, 561, 643, 343, 17, 184, 568, 722, 434, 692, 329, 334, 342, 477, 179, 288, 615, 71, 154, 625, 548, 579, 631, 421, 709, 336, 570, 544, 596, 564, 313, 556, 613, 386, 35, 468, 57, 679, 345, 765, 398, 127, 567, 700, 173, 757, 268, 113, 164, 200, 413, 741, 379, 214, 392, 224, 276, 132, 767, 665, 346, 416, 645, 112, 97, 306, 269, 395, 211, 100, 215, 195, 529, 130, 320, 15, 105, 732, 494, 299, 472, 64, 505, 668, 96, 716, 617, 404, 20, 363, 662, 539, 309, 369, 463, 243, 531, 701, 92, 381, 360, 660, 495, 491, 666, 149, 212, 272, 146, 271, 70, 162, 651, 77, 292, 753, 142, 172, 95, 562, 75, 291, 749, 169, 339, 414, 740, 408, 533, 474, 367, 316, 4, 133, 536, 368, 480, 131, 317, 14, 37, 466, 315, 114, 597, 393, 736, 143, 761, 750, 157, 621, 703, 746, 516, 747, 204, 262, 26, 657, 88, 232, 310, 102, 648, 238, 273, 577, 383, 632, 489, 664, 55, 649, 2, 51, 686, 538, 391, 48, 264, 78, 453, 555, 407, 47, 553, 557, 59, 227, 431, 694, 541, 580, 355, 34, 5, 39, 752
};

/****************
history constants
****************/

constexpr int PAWN_CORRHIST_SIZE = 16384;
constexpr int MINOR_CORRHIST_SIZE = 16384;
constexpr int MAJOR_CORRHIST_SIZE = 16384;
constexpr int NONPAWN_CORRHIST_SIZE = 16384;


/**************
chess constants
**************/

constexpr int NUM_COLORS = 2;
constexpr int NUM_PIECES = 12;
constexpr int NUM_PIECETYPES = 6;
constexpr int NUM_SQUARES = 64;

/****************
general constants
****************/

constexpr int NUM_GENFENS_RANDOM_MOVES = 10;

constexpr int TT_MIN_SIZE = 2;
constexpr int TT_MAX_SIZE = 4096;

constexpr int MAX_PLY = 256;
constexpr int STACK_PADDING_SIZE = 2;

constexpr int BENCHMARK_DEPTH = 12;
constexpr int LONG_BENCHMARK_DEPTH = 18;

constexpr int ENGINE_MAX_DEPTH = 255;

constexpr int DEPTH_UNSEARCHED = -1;
constexpr int DEPTH_QSEARCH = 0;

constexpr int QSEARCH_SOFT_DEPTH_LIMIT = 6;
constexpr int QSEARCH_HARD_DEPTH_LIMIT = 15;

/*****
scores
*****/

constexpr int BEST_MOVE_SCORE = 100'000;
constexpr int WORST_MOVE_SCORE = -100'000;

constexpr int MAX_MATE_PLY = 200;

constexpr int BEST_VALUE = 32'498;
constexpr int TB_VALUE = 32'499;
constexpr int MATE_VALUE = 32'700;
constexpr int INFINITE_VALUE = 32'701;
constexpr int NO_VALUE = 32'702;

constexpr int SEE_KING_VALUE = 150'000;

// |    value            |          name                    | is_valid  | is_mate   | is_decisive |
// ================================================================================================
// |       ...           |       xxx                        |   crash   | crash     | crash
// |      -32702         | -NO_VALUE                        |   false   | false     | crash
// |      -32701         | -INFINITE_VALUE                  |   false   | false     | crash
// |      -32700         | -MATE_VALUE (mate in 0)          |   true    | true      | true
// | -32699, ..., -32501 |       ...                        |   true    | true      | true
// |      -32500         | -MATE_VALUE+200 (mate in 200)    |   true    | true      | true
// |      -32499         | -TB_VALUE                        |   true    | false     | true

// |      -32498         | -BEST_VALUE                      |   true    | false     | false
// | -32498, ..., 32498  | valid eval range                 |   true    | false     | false
// |       32498         | BEST_VALUE                       |   true    | false     | false

// |       32499         | TB_VALUE                         |   true    | false     | true
// |       32500         | MATE_VALUE-200 (mate in 200)     |   true    | true      | true
// |  32501, ..., 32699  |       ...                        |   true    | true      | true
// |       32700         | MATE_VALUE (mate in 0)           |   true    | true      | true
// |       32701         | INFINITE_VALUE                   |   false   | false     | crash
// |       32702         | NO_VALUE                         |   false   | false     | crash
// |       ...           |       xxx                        |   crash   | crash     | crash

constexpr int is_valid(int value){
    assert(std::abs(value) <= NO_VALUE);
    return std::abs(value) < INFINITE_VALUE;
}

constexpr bool is_mate(int value){
    assert(std::abs(value) <= NO_VALUE);
    return (std::abs(value) >= MATE_VALUE - MAX_MATE_PLY && std::abs(value) <= MATE_VALUE);
}

constexpr bool is_win(int value){
    assert(is_valid(value));
    return value >= TB_VALUE;
}

constexpr bool is_loss(int value){
    assert(is_valid(value));
    return value <= -TB_VALUE;
}

constexpr bool is_decisive(int value){
    return is_win(value) || is_loss(value);
}

constexpr bool is_regular_eval(int value, bool zws_safe = true){
    assert(std::abs(value) <= NO_VALUE);
    return std::abs(value) <= BEST_VALUE - (zws_safe ? 1 : 0);
}
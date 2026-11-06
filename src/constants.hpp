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
    493, 37, 408, 20, 597, 2, 217, 570, 50, 204, 14, 529, 154, 698, 283, 248, 343, 359, 593, 532, 461, 114, 93, 272, 207, 565, 423, 250, 110, 99, 723, 685, 725, 650, 43, 295, 454, 760, 614, 527, 29, 616, 116, 206, 512, 425, 198, 689, 197, 332, 595, 716, 600, 407, 556, 737, 323, 491, 444, 590, 657, 440, 414, 451, 196, 70, 155, 662, 31, 278, 237, 541, 502, 665, 615, 428, 280, 0, 436, 747, 501, 102, 728, 142, 526, 361, 40, 74, 411, 383, 638, 477, 41, 560, 582, 77, 678, 163, 219, 82, 763, 764, 612, 547, 351, 757, 762, 328, 168, 23, 66, 51, 625, 396, 251, 236, 572, 684, 621, 124, 713, 687, 717, 347, 691, 61, 632, 750, 324, 536, 184, 705, 683, 83, 16, 379, 160, 634, 260, 213, 59, 216, 68, 707, 575, 699, 247, 262, 246, 55, 10, 7, 145, 267, 273, 552, 418, 639, 631, 554, 430, 469, 620, 364, 319, 741, 211, 339, 546, 200, 369, 203, 132, 500, 459, 668, 484, 591, 589, 177, 481, 636, 8, 174, 519, 382, 722, 533, 64, 373, 494, 121, 721, 528, 358, 669, 486, 353, 751, 183, 580, 525, 202, 130, 224, 144, 126, 268, 555, 543, 117, 47, 749, 305, 1, 296, 357, 133, 442, 697, 78, 220, 569, 736, 602, 242, 534, 680, 427, 352, 128, 195, 574, 24, 419, 86, 603, 274, 594, 378, 375, 649, 478, 26, 681, 661, 466, 677, 391, 245, 710, 463, 719, 188, 398, 752, 514, 380, 348, 400, 405, 734, 517, 191, 333, 740, 557, 389, 326, 492, 498, 441, 587, 674, 317, 261, 627, 201, 539, 645, 524, 520, 702, 583, 4, 140, 331, 464, 714, 156, 345, 598, 402, 568, 596, 559, 148, 223, 422, 508, 178, 670, 9, 521, 215, 18, 258, 299, 522, 76, 622, 613, 327, 259, 162, 186, 294, 150, 695, 164, 474, 172, 147, 733, 399, 243, 42, 537, 314, 712, 629, 432, 141, 279, 334, 421, 315, 404, 700, 372, 62, 192, 472, 91, 210, 497, 190, 450, 588, 767, 320, 122, 95, 252, 666, 393, 284, 406, 97, 611, 513, 44, 69, 690, 264, 111, 75, 244, 282, 516, 381, 221, 455, 329, 227, 80, 316, 744, 301, 363, 15, 269, 761, 585, 56, 672, 564, 452, 152, 158, 112, 356, 226, 298, 468, 387, 325, 53, 748, 330, 617, 605, 161, 230, 608, 263, 766, 365, 647, 214, 573, 169, 671, 624, 350, 496, 27, 397, 6, 673, 275, 271, 609, 173, 535, 619, 676, 374, 205, 232, 307, 759, 628, 706, 54, 107, 732, 548, 626, 120, 385, 344, 426, 354, 79, 467, 115, 313, 181, 566, 658, 558, 276, 660, 726, 212, 139, 362, 538, 235, 438, 745, 561, 507, 94, 667, 542, 143, 518, 87, 127, 36, 765, 346, 3, 644, 187, 194, 435, 485, 33, 604, 610, 113, 449, 623, 171, 30, 11, 403, 730, 52, 753, 641, 577, 417, 453, 675, 138, 104, 135, 368, 302, 544, 233, 476, 456, 470, 578, 445, 549, 311, 758, 322, 32, 377, 654, 199, 81, 718, 579, 119, 409, 439, 504, 208, 45, 756, 58, 545, 309, 270, 384, 735, 410, 473, 149, 101, 5, 462, 108, 106, 479, 222, 563, 229, 19, 571, 57, 137, 540, 134, 708, 12, 412, 434, 254, 255, 170, 34, 157, 136, 715, 651, 550, 701, 531, 448, 688, 277, 696, 679, 85, 562, 306, 693, 310, 312, 664, 231, 159, 180, 754, 129, 49, 253, 303, 297, 648, 371, 292, 285, 711, 640, 487, 720, 63, 643, 366, 166, 84, 415, 606, 663, 286, 447, 446, 635, 103, 738, 218, 460, 692, 742, 105, 482, 394, 240, 746, 175, 530, 48, 228, 704, 25, 291, 281, 506, 234, 652, 655, 599, 151, 287, 633, 249, 238, 592, 185, 420, 709, 510, 39, 511, 35, 637, 437, 167, 239, 176, 335, 340, 390, 386, 433, 67, 457, 88, 376, 290, 431, 480, 349, 341, 338, 576, 413, 515, 682, 179, 46, 289, 131, 729, 488, 642, 458, 586, 225, 193, 618, 100, 370, 401, 17, 241, 553, 471, 71, 743, 65, 392, 475, 505, 367, 355, 123, 146, 694, 465, 257, 503, 318, 73, 118, 646, 686, 98, 13, 656, 653, 308, 22, 551, 581, 607, 72, 724, 28, 584, 443, 755, 659, 266, 256, 342, 21, 490, 265, 89, 567, 90, 416, 165, 489, 731, 703, 304, 96, 388, 360, 209, 153, 495, 125, 189, 395, 499, 739, 38, 321, 337, 424, 523, 601, 429, 109, 336, 288, 300, 293, 92, 630, 60, 727, 182, 509, 483
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
constexpr int GENFENS_FILTER_DEPTH = 9;
constexpr int GENFENS_MAX_VALUE = 600;

constexpr int TT_MIN_SIZE = 2;
constexpr int TT_MAX_SIZE = 1048576;

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
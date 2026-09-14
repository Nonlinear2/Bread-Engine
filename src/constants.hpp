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
    436, 128, 178, 523, 531, 32, 304, 71, 102, 481, 528, 671, 120, 601, 511, 757, 21, 158, 238, 312, 634, 63, 90, 190, 723, 746, 96, 257, 9, 171, 392, 402, 411, 287, 459, 718, 1, 84, 28, 85, 111, 151, 568, 97, 415, 487, 398, 479, 681, 697, 762, 38, 397, 29, 83, 229, 299, 507, 533, 708, 739, 41, 44, 412, 586, 570, 656, 67, 261, 410, 682, 709, 255, 326, 362, 456, 476, 498, 666, 20, 322, 354, 423, 155, 170, 435, 526, 628, 50, 117, 374, 576, 454, 66, 107, 356, 687, 55, 141, 310, 473, 711, 729, 10, 661, 244, 388, 443, 159, 247, 363, 419, 463, 605, 167, 209, 483, 17, 62, 223, 306, 332, 383, 422, 529, 731, 3, 259, 636, 243, 289, 441, 474, 747, 328, 394, 222, 434, 515, 635, 750, 135, 336, 337, 466, 560, 592, 627, 153, 316, 370, 535, 596, 706, 98, 564, 631, 30, 79, 196, 391, 595, 602, 56, 94, 168, 407, 584, 753, 241, 424, 449, 732, 8, 103, 132, 172, 192, 260, 268, 513, 667, 15, 401, 684, 751, 34, 207, 335, 409, 562, 12, 288, 464, 727, 195, 360, 616, 662, 122, 521, 581, 54, 235, 668, 263, 510, 522, 693, 743, 245, 278, 621, 185, 147, 181, 742, 88, 495, 548, 358, 425, 587, 594, 688, 201, 272, 333, 343, 36, 157, 556, 660, 355, 505, 269, 733, 760, 23, 47, 112, 438, 323, 471, 492, 501, 555, 19, 218, 707, 720, 42, 242, 338, 725, 494, 567, 637, 277, 421, 611, 674, 386, 502, 604, 622, 129, 279, 7, 538, 540, 728, 559, 414, 610, 318, 339, 665, 109, 124, 215, 294, 385, 465, 579, 745, 765, 25, 405, 698, 701, 198, 433, 615, 16, 365, 583, 669, 717, 740, 756, 58, 359, 319, 480, 629, 703, 754, 73, 174, 300, 5, 462, 734, 6, 95, 491, 542, 72, 4, 213, 298, 519, 534, 759, 116, 673, 349, 448, 566, 646, 39, 378, 182, 350, 575, 518, 78, 262, 509, 161, 162, 281, 381, 275, 347, 582, 11, 65, 234, 633, 145, 377, 426, 517, 550, 589, 724, 296, 361, 444, 680, 744, 146, 193, 297, 205, 558, 561, 761, 33, 253, 303, 393, 420, 285, 593, 18, 186, 384, 51, 232, 224, 123, 467, 619, 315, 514, 546, 165, 496, 574, 183, 670, 719, 380, 40, 320, 408, 446, 427, 472, 551, 59, 330, 240, 291, 368, 544, 726, 57, 654, 152, 271, 382, 485, 86, 150, 77, 113, 115, 149, 226, 530, 580, 118, 295, 246, 250, 696, 764, 2, 233, 664, 46, 187, 13, 282, 307, 321, 482, 532, 53, 230, 648, 89, 142, 373, 237, 431, 239, 490, 48, 91, 191, 344, 644, 110, 590, 22, 64, 404, 175, 267, 612, 221, 525, 108, 399, 606, 695, 265, 545, 599, 735, 453, 468, 714, 722, 81, 214, 651, 692, 439, 428, 766, 340, 678, 700, 188, 432, 445, 75, 220, 249, 313, 396, 657, 429, 537, 348, 76, 280, 493, 607, 45, 101, 136, 689, 166, 500, 679, 140, 273, 302, 626, 82, 331, 554, 675, 352, 390, 206, 549, 447, 504, 499, 324, 539, 624, 652, 131, 137, 357, 749, 686, 200, 375, 470, 741, 144, 645, 643, 699, 251, 663, 106, 691, 387, 212, 341, 458, 649, 430, 27, 31, 37, 70, 121, 484, 638, 14, 755, 541, 127, 716, 301, 179, 210, 758, 24, 176, 690, 216, 655, 290, 228, 138, 225, 61, 104, 400, 475, 264, 460, 314, 126, 156, 163, 284, 403, 632, 345, 547, 486, 730, 60, 270, 366, 283, 372, 640, 524, 659, 342, 585, 520, 92, 573, 125, 748, 571, 712, 325, 236, 469, 346, 406, 647, 552, 376, 379, 455, 565, 99, 160, 641, 169, 177, 569, 478, 609, 139, 577, 597, 578, 710, 653, 309, 353, 256, 317, 437, 105, 440, 620, 311, 536, 49, 702, 0, 164, 231, 305, 252, 614, 258, 208, 763, 416, 721, 189, 713, 199, 452, 413, 694, 133, 503, 35, 173, 184, 293, 650, 395, 553, 685, 598, 254, 705, 608, 119, 658, 488, 543, 364, 508, 68, 308, 26, 512, 371, 736, 623, 489, 497, 516, 676, 704, 683, 506, 148, 450, 642, 180, 572, 752, 625, 738, 603, 451, 618, 134, 329, 369, 737, 477, 613, 154, 630, 204, 672, 715, 677, 227, 203, 87, 367, 418, 461, 202, 197, 389, 217, 274, 442, 617, 588, 457, 52, 130, 100, 563, 767, 248, 292, 114, 351, 219, 74, 211, 286, 639, 69, 527, 327, 143, 600, 266, 276, 334, 591, 80, 194, 93, 417, 43, 557
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
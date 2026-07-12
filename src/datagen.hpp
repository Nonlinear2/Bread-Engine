#pragma once

#include <fstream>
#include <random>
#include "chess.hpp"
#include "constants.hpp"
#include "core.hpp"

namespace Datagen {

using namespace chess;

void genfens(Engine& engine, std::mt19937 rng, int count);

} // namespace Benchmark
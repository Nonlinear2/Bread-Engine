#include "core.hpp"
#include "nnue_misc.hpp"
#include <fstream>

namespace Benchmark {

using namespace std::chrono;

int64_t sum(std::vector<int> values);
void benchmark_nn();
void benchmark_engine(Engine& engine, int depth);
void benchmark_ft_activation();

} // namespace Benchmark
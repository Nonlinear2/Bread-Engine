#include "core.hpp"

namespace Benchmark {

int64_t sum(std::vector<int> values);
void benchmark_nn();
void benchmark_engine(Engine& engine, int depth);

} // namespace Benchmark
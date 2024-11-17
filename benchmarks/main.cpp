#include <benchmarks/bench_utils.hpp>

#include <rexcore/allocators.hpp>

int main()
{
	RexCore::StartTrackingMemory();
	RexCore::Benchmark::RegisterBenchmark::RunBenchmarks();
	return RexCore::CheckForLeaks();
}
#include <benchmarks/bench_utils.hpp>

#include <rexcore/allocators.hpp>

using namespace RexCore;

BENCHMARK("Allocators")
{
	static constexpr U64 N = 50'000;
	void** ptrs = new void* [N];

	MallocAllocator rexMalloc;
	BENCH_LOOP("Rex Malloc", 1'000, N, {
		for (int i = 0; i < N; i++)
		{
			ptrs[i] = rexMalloc.Allocate(32, 8);
		}

		for (int i = 0; i < N; i++)
		{
			rexMalloc.Free(ptrs[i], 32);
		}
	});

	ArenaAllocator arena;
	BENCH_LOOP("Rex Arena", 1'000, N, {
		for (int i = 0; i < N; i++)
		{
			ptrs[i] = arena.Allocate(32, 8);
		}

		for (int i = 0; i < N; i++)
		{
			arena.Free(ptrs[i], 32);
		}
		arena.Reset();
	});

	BENCH_LOOP("Malloc", 1'000, N, {
		for (int i = 0; i < N; i++)
		{
			ptrs[i] = malloc(32);
		}

		for (int i = 0; i < N; i++)
		{
			free(ptrs[i]);
		}
	});

	BENCH_LOOP("Aligned Malloc", 1'000, N, {
		for (int i = 0; i < N; i++)
		{
			ptrs[i] = _aligned_malloc(32, 8);
		}

		for (int i = 0; i < N; i++)
		{
			_aligned_free(ptrs[i]);
		}
	});

	PoolAllocatorBase<32, 8> pool;
	BENCH_LOOP("Pool", 1'000, N, {
		for (int i = 0; i < N; i++)
		{
			ptrs[i] = pool.Allocate(32, 8);
		}

		for (int i = 0; i < N; i++)
		{
			pool.Free(ptrs[i], 32);
		}
	});

	delete[] ptrs;
}
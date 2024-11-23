#pragma once

#include <vector>
#include <algorithm>

#include <rexcore/time.hpp>
#include <rexcore/core.hpp>

namespace RexCore::Benchmark
{
	class RegisterBenchmark
	{
	public:
		RegisterBenchmark(const char* name, void (*body)())
		{
			s_benchmarks.emplace_back(name, body);
		}

		static void RunBenchmarks()
		{
			std::sort(s_benchmarks.begin(), s_benchmarks.end(), [](const Benchmark& a, const Benchmark& b) { return strcmp(a.name, b.name) < 0; });

			for (const Benchmark& bench : s_benchmarks)
			{
				printf("[%s]\n", bench.name);
				bench.body();
			}
		}

	private:
		struct Benchmark
		{
			const char* name;
			void (*body)();
		};

		inline static std::vector<Benchmark> s_benchmarks;
	};

	class ScopeTimer
	{
	public:
		ScopeTimer(const char* name, U64 count)
			: m_name(name)
			, m_count(static_cast<double>(count))
			, m_sw()
		{}

		~ScopeTimer()
		{
			printf("    %s : %.3f ns\n", m_name, static_cast<double>(m_sw.ElapsedNs()) / m_count);
		}

	private:
		const char* m_name;
		double m_count;
		Stopwatch m_sw;
	};

#define CONCAT(a, b) CONCAT_INNER(a, b)
#define CONCAT_INNER(a, b) a ## b
#define DEBUG_BREAK() __debugbreak()

#define BENCH_INNER(name, funcName) \
	static void funcName(); \
	static ::RexCore::Benchmark::RegisterBenchmark CONCAT(bench_, __COUNTER__) = ::RexCore::Benchmark::RegisterBenchmark(name, funcName); \
	static void funcName()

#define BENCHMARK(name) BENCH_INNER(name, CONCAT(bench_, __COUNTER__))

#define BENCH_LOOP(name, count, extra_divisor, body) \
	{ \
		REX_CORE_TRACE_NAMED(name); \
		::RexCore::Benchmark::ScopeTimer timer(name, U64(count) * U64(extra_divisor)); \
		for (int bench_i = 0; bench_i < count; bench_i++) \
			body; \
	}
}
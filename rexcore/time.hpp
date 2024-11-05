#pragma once
#include <rexcore/core.hpp>

#include <chrono>

namespace RexCore
{
	class Stopwatch
	{
	public:
		Stopwatch()
		{
			Restart();
		}

		void Restart()
		{
			m_startTime = std::chrono::high_resolution_clock::now();
		}

		U64 ElapsedNs()
		{
			auto end = std::chrono::high_resolution_clock::now();
			return static_cast<U64>(std::chrono::duration_cast<std::chrono::nanoseconds>(end - m_startTime).count());
		}

	private:
		std::chrono::high_resolution_clock::time_point m_startTime;
	};
}
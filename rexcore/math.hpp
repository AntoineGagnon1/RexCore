#pragma once

#include <concepts>
#include <limits>

namespace RexCore::Math
{
	// Ceil(a / b), for positive integers only
	template<std::unsigned_integral T>
	constexpr T CeilDiv(T a, T b)
	{
		return (a + b - 1) / b;
	}

	template<typename T>
	constexpr T Min(T a, T b)
	{
		return a < b ? a : b;
	}

	template<typename T>
	constexpr T Max(T a, T b)
	{
		return a > b ? a : b;
	}

	template<std::integral T>
	constexpr T MinValue()
	{
		return std::numeric_limits<T>::min();
	}

	template<std::integral T>
	constexpr T MaxValue()
	{
		return std::numeric_limits<T>::max();
	}
}
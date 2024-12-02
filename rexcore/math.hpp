#pragma once

#include <concepts>
#include <limits>
#include <bit>

namespace RexCore::Math
{
	// Ceil(a / b), for positive integers only
	template<std::unsigned_integral T>
	constexpr T CeilDiv(T a, T b) noexcept
	{
		return (a + b - 1) / b;
	}

	template<typename T>
	constexpr T Min(T a, T b) noexcept
	{
		return a < b ? a : b;
	}

	template<typename T>
	constexpr T Max(T a, T b) noexcept
	{
		return a > b ? a : b;
	}

	template<typename T>
	constexpr T Abs(T a) noexcept
	{
		return a >= static_cast<T>(0) ? a : -a;
	}

	template<std::integral T>
	constexpr T MinValue() noexcept
	{
		return std::numeric_limits<T>::min();
	}

	template<std::integral T>
	constexpr T MaxValue() noexcept
	{
		return std::numeric_limits<T>::max();
	}

	template<std::integral T>
	constexpr T NextPowerOfTwo(T v) noexcept
	{
		return std::bit_ceil(v);
	}

	template<std::integral T>
	constexpr T PreviousPowerOfTwo(T v) noexcept
	{
		return std::bit_floor(v);
	}
}
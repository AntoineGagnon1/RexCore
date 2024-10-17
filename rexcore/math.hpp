#pragma once

#include <concepts>

namespace RexCore::Math
{
	// Ceil(a / b), for positive integers only
	template<std::unsigned_integral T>
	constexpr T CeilDiv(T a, T b)
	{
		return (a + b - 1) / b;
	}
}
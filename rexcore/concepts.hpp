#pragma once

#include <type_traits>

namespace RexCore
{
	template<typename T>
	concept IClonable = std::is_copy_assignable_v<T> || std::is_copy_constructible_v<T> || requires(const T a)
	{
		{ a.Clone() } -> std::convertible_to<T>;
	};

	template<IClonable T>
	[[nodiscard]] inline T Clone(const T& obj)
	{
		if constexpr (std::is_copy_constructible_v<T>)
			return T(obj);
		else if constexpr (std::is_copy_assignable_v<T>)
			return obj;
		else
			return obj.Clone();
	}
}
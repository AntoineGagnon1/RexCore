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

	template<IClonable T>
	inline void CloneInto(const T& obj, T* into)
	{
		new (into) T(std::move(RexCore::Clone<T>(obj)));
	}

	template<typename PredT, typename ...Args>
	concept IPredicate = requires(const PredT p, Args ...args)
	{
		{ p(args...) } -> std::convertible_to<bool>;
	};

	// To use for references call CopyConst<T, U>& instead of CopyConst<T, U&>
	template<typename From, typename To>
	using CopyConst = std::conditional_t<std::is_const_v<std::remove_reference_t<From>>, std::add_const_t<To>, To>;

	template<typename T>
	concept ITrivialyMoveable = std::is_trivially_move_assignable_v<T> && std::is_trivially_move_constructible_v<T>;
}
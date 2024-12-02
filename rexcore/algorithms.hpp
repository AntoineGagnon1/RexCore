#pragma once

#include <algorithm>

namespace RexCore
{
	template<typename ArrayT>
	constexpr void Sort(ArrayT& array)
	{
		std::sort(array.Begin(), array.End());
	}

	template<typename ArrayT, typename PredT>
	constexpr void Sort(ArrayT& array, PredT&& pred)
	{
		std::sort(array.Begin(), array.End(), std::forward<PredT>(pred));
	}

	// Predicate : void(const T& elem, ResultT& result)
	template<typename ResultT, typename ArrayT, typename PredT>
	constexpr ResultT Reduce(ArrayT& array, PredT&& pred)
	{
		ResultT result{};
		for (const auto& elem : array)
		{
			pred(elem, result);
		}

		return result;
	}

	// Predicate : ResultT(const T& elem)
	template<typename ResultT, typename ArrayT, typename PredT>
	constexpr ResultT Sum(ArrayT& array, PredT&& pred)
	{
		ResultT result{};
		for (const auto& elem : array)
		{
			result += pred(elem);
		}

		return result;
	}
}
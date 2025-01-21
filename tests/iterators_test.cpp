#include <tests/test_utils.hpp>

#include <rexcore/containers/vector.hpp>
#include <rexcore/iterators.hpp>

using namespace RexCore;

TEST_CASE("Iterators/Zip")
{
	InplaceVector<int, 8> vec1{ 1, 2, 3, 4, 5, 6, 7, 8 };
	InplaceVector<int, 8> vec2{ 1, 2, 3, 4, 5, 6, 7, 8 };
	const InplaceVector<int, 8> vec3{ 1, 2, 3, 4, 5, 6, 7, 8 };

	for (auto [a, b, c, d, e] : Iter::Zip(vec1, vec2, vec3, vec1, Iter::Zip(vec1, vec3)))
	{
		ASSERT(a + b + c + d == a * 4);
	}
}

TEST_CASE("Iterators/Enumerate")
{
	InplaceVector<int, 8> vec1{ 1, 2, 3, 4, 5, 6, 7, 8 };
	const InplaceVector<int, 8> vec2{ 1, 2, 3, 4, 5, 6, 7, 8 };

	for (auto[i, v] : Iter::Enumerate(vec1))
	{
		ASSERT(i + 1 == v);
	}

	for (auto [i, v] : Iter::Enumerate(vec2))
	{
		ASSERT(i + 1 == v);
	}

	for (auto [i, v1, v2] : Iter::Enumerate(vec1, vec2))
	{
		ASSERT(i + 1 == v1);
		ASSERT(i + 1 == v2);
	}
}

TEST_CASE("Iterators/Skip")
{
	InplaceVector<int, 8> vec1{ 1, 2, 3, 4, 5, 6, 7, 8 };
	const InplaceVector<int, 8> vec2{ 1, 2, 3, 4, 5, 6, 7, 8 };

	for (auto [i, v] : Iter::Enumerate(Iter::Skip(3, vec1)))
	{
		ASSERT(i + 4 == v);
	}

	for (auto [i, v] : Iter::Enumerate(Iter::Skip(0, vec1)))
	{
		ASSERT(i + 1 == v);
	}

	for ([[maybe_unused]] auto _ : Iter::Skip(8, vec1))
	{
		ASSERT(false);
	}

	for ([[maybe_unused]] auto _ : Iter::Skip(10, vec2))
	{
		ASSERT(false);
	}

	for (auto [i, v] : Iter::Enumerate(Iter::Skip(3, vec2)))
	{
		ASSERT(i + 4 == v);
	}
}

TEST_CASE("Iterators/Combinations")
{
	InplaceVector<int, 8> vec1{ 1, 2, 3, 4, 5, 6, 7, 8 };
	const InplaceVector<int, 8> vec2{ 1, 2, 3, 4, 5, 6, 7, 8 };

	for (auto [i, v] : Iter::Enumerate(Iter::Skip(1, vec1)))
	{
		ASSERT(i + 2 == v);
	}

	for (auto [i, v] : Iter::Skip(1, Iter::Enumerate(vec1)))
	{
		ASSERT(i + 1 == v);
	}

	for (auto [i, v] : Iter::Enumerate(Iter::Zip(vec1, vec2)))
	{
		ASSERT(i + 1 == std::get<0>(v));
		ASSERT(i + 1 == std::get<1>(v));
	}

	for (auto [v1, v2] : Iter::Zip(Iter::Enumerate(vec1), Iter::Enumerate(vec2)))
	{
		ASSERT(std::get<0>(v1) + 1 == std::get<1>(v1));
		ASSERT(std::get<0>(v2) + 1 == std::get<1>(v2));
	}

	for (auto [v1, v2] : Iter::Zip(Iter::Skip(1, vec1), Iter::Skip(2, vec2)))
	{
		ASSERT(v1 + 1 == v2);
	}

	for (auto [v1, v2] : Iter::Skip(1, Iter::Zip(vec1, vec2)))
	{
		ASSERT(v1 == v2 && v1 >= 2);
	}
}
#include <tests/test_utils.hpp>

#include <rexcore/containers/vector.hpp>

using namespace RexCore;

class MoveOnlyType
{
public:
	MoveOnlyType(MoveOnlyType&& other) = default;
	MoveOnlyType& operator=(MoveOnlyType&& other) = default;

	MoveOnlyType(const MoveOnlyType&) = delete;
	MoveOnlyType& operator=(const MoveOnlyType&) = delete;

	MoveOnlyType(int value) : value(value) {}

	operator U32() const { return value; }

	int value;
};

template<std::unsigned_integral IndexT>
static void TestVector()
{
	{
		VectorBase<U32, IndexT, DefaultAllocator> vec;
		ASSERT(vec.IsEmpty());
		ASSERT(vec.Size() == 0);
		ASSERT(vec.Capacity() == 0);
		ASSERT(vec.Data() == nullptr);

		vec.PushBack(1);
		ASSERT(!vec.IsEmpty());
		ASSERT(vec.Size() == 1);
		ASSERT(vec.Capacity() == 8);
		ASSERT(vec.Data() != nullptr);
		ASSERT(vec.First() == 1);
		ASSERT(vec.Last() == 1);

		for (U32 i = 0; i < 100; i++)
		{
			vec.EmplaceBack(i);
			ASSERT(!vec.IsEmpty());
			ASSERT(vec.Size() == i + 2);
			ASSERT(vec.Capacity() >= vec.Size());
			ASSERT(vec.Data() != nullptr);
			ASSERT(vec.Last() == i);
			ASSERT(vec[(IndexT)i + 1] == i);
		}

		for (S32 i = 99; i >= 0; i--)
		{
			ASSERT(vec.PopBack() == (U32)i);
		}

		vec.Clear();
		ASSERT(vec.IsEmpty());
		ASSERT(vec.Size() == 0);
		ASSERT(vec.Capacity() > 0);
		ASSERT(vec.Data() != nullptr);

		vec.Free();
		ASSERT(vec.IsEmpty());
		ASSERT(vec.Size() == 0);
		ASSERT(vec.Capacity() == 0);
		ASSERT(vec.Data() == nullptr);

		vec.PushBack(1);
		vec.PushBack(3);
		vec.PushBack(4);
		vec.InsertAt(1, 2);

		vec.RemoveAt(1);
		ASSERT(vec.Size() == 3);
		ASSERT(vec[0] == 1);
		ASSERT(vec[1] == 4);
		ASSERT(vec[2] == 3);

		vec.EmplaceAt(1, 2);
		vec.RemoveAtOrdered(0);
		ASSERT(vec.Size() == 3);
		ASSERT(vec[0] == 2);
		ASSERT(vec[1] == 4);
		ASSERT(vec[2] == 3);
	}

	// Now the same thing but with a complex type, string
	{
		VectorBase<MoveOnlyType, IndexT, DefaultAllocator> vec;
		ASSERT(vec.IsEmpty());
		ASSERT(vec.Size() == 0);
		ASSERT(vec.Capacity() == 0);
		ASSERT(vec.Data() == nullptr);

		vec.EmplaceBack(1);
		ASSERT(!vec.IsEmpty());
		ASSERT(vec.Size() == 1);
		ASSERT(vec.Capacity() == 8);
		ASSERT(vec.Data() != nullptr);
		ASSERT(vec.First() == 1);
		ASSERT(vec.Last() == 1);

		for (U32 i = 0; i < 100; i++)
		{
			vec.EmplaceBack(i);
			ASSERT(!vec.IsEmpty());
			ASSERT(vec.Size() == i + 2);
			ASSERT(vec.Capacity() >= vec.Size());
			ASSERT(vec.Data() != nullptr);
			ASSERT(vec.Last() == i);
			ASSERT(vec[(IndexT)i + 1] == i);
		}

		for (S32 i = 99; i >= 0; i--)
		{
			ASSERT(vec.PopBack() == (U32)i);
		}

		vec.Clear();
		ASSERT(vec.IsEmpty());
		ASSERT(vec.Size() == 0);
		ASSERT(vec.Capacity() > 0);
		ASSERT(vec.Data() != nullptr);

		vec.Free();
		ASSERT(vec.IsEmpty());
		ASSERT(vec.Size() == 0);
		ASSERT(vec.Capacity() == 0);
		ASSERT(vec.Data() == nullptr);

		vec.EmplaceBack(1);
		vec.EmplaceBack(2);
		vec.EmplaceBack(3);

		vec.RemoveAt(1);
		ASSERT(vec.Size() == 2);
		ASSERT(vec[0] == 1);
		ASSERT(vec[1] == 3);

		vec.EmplaceBack(4);
		vec.RemoveAtOrdered(0);
		ASSERT(vec.Size() == 2);
		ASSERT(vec[0] == 3);
		ASSERT(vec[1] == 4);
	}
}

TEST_CASE("Containers/Vector")
{
	TestVector<U16>(); // SmallVector
	TestVector<U32>(); // Vector
	TestVector<U64>(); // BigVector
}
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

template<typename VecT, bool MoveOnly>
static void TestVector()
{
	using IndexT = VecT::IndexType;
	VecT vec = VecT();
	ASSERT(vec.IsEmpty());
	ASSERT(vec.Size() == 0);
	ASSERT(vec.Capacity() == 0);
	ASSERT(vec.Data() == nullptr);

	if constexpr (MoveOnly)
		vec.EmplaceBack(1);
	else
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

	vec.EmplaceBack(1);
	vec.EmplaceBack(3);
	vec.EmplaceBack(4);

	if constexpr (MoveOnly)
		vec.EmplaceAt(1, 2);
	else
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

template<typename VecT, bool MoveOnly>
static void TestInplaceVector()
{
	using IndexT = VecT::IndexType;
	VecT vec = VecT();
	ASSERT(vec.IsEmpty());
	ASSERT(vec.Size() == 0);
	ASSERT(vec.Capacity() == 16);
	ASSERT(vec.Data() != nullptr);

	if constexpr (MoveOnly)
		vec.EmplaceBack(1);
	else
		vec.PushBack(1);
	ASSERT(!vec.IsEmpty());
	ASSERT(vec.Size() == 1);
	ASSERT(vec.Capacity() == 16);
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
	vec.EmplaceBack(3);
	vec.EmplaceBack(4);

	if constexpr (MoveOnly)
		vec.EmplaceAt(1, 2);
	else
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

TEST_CASE("Containers/Vector")
{
	TestVector<SmallVector<U32>, false>();
	TestVector<SmallVector<MoveOnlyType>, true>();
	TestVector<Vector<U32>, false>();
	TestVector<Vector<MoveOnlyType>, true>();
	TestVector<BigVector<U32>, false>();
	TestVector<BigVector<MoveOnlyType>, true>();

	TestInplaceVector<SmallInplaceVector<U32, 16>, false>();
	TestInplaceVector<SmallInplaceVector<MoveOnlyType, 16>, true>();
	TestInplaceVector<InplaceVector<U32, 16>, false>();
	TestInplaceVector<InplaceVector<MoveOnlyType, 16>, true>();
	TestInplaceVector<BigInplaceVector<U32, 16>, false>();
	TestInplaceVector<BigInplaceVector<MoveOnlyType, 16>, true>();
}
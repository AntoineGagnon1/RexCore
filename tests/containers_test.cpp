#include <tests/test_utils.hpp>

#include <rexcore/containers/vector.hpp>
#include <rexcore/math.hpp>

using namespace RexCore;

class MoveOnlyType
{
public:
	MoveOnlyType(MoveOnlyType&& other) = default;
	MoveOnlyType& operator=(MoveOnlyType&& other) = default;

	MoveOnlyType(const MoveOnlyType&) = delete;
	MoveOnlyType& operator=(const MoveOnlyType&) = delete;

	MoveOnlyType(U32 value) : value(value) {}

	operator U32() const { return value; }

	MoveOnlyType Clone() const
	{
		return MoveOnlyType(value);
	}

	U32 value;
};

template<typename VecT>
void VecTestReserve()
{
	using IndexT = VecT::IndexType;
	VecT vec = VecT();

	IndexT oldSize = vec.Size();
	IndexT newCapacity = (vec.Capacity() + 1) * 2;
	
	vec.Reserve(newCapacity);
	vec.Reserve(0); // Should do nothing
	ASSERT(vec.Capacity() >= newCapacity);
	ASSERT(vec.Size() == oldSize);
	ASSERT(vec.Data() != nullptr);
	ASSERT(vec.IsEmpty() == (oldSize == 0));

	auto ptr = vec.Data();
	vec.Reserve(newCapacity); // Make sure that reserving to the current capacity does not reallocate
	ASSERT(vec.Data() == ptr);

	for (IndexT i = 0; i < newCapacity; i++)
		vec.EmplaceBack((U32)i);

	vec.Reserve(vec.Capacity() * 2); // Make sure that reserving does not change the content
	ASSERT(vec.Data() != ptr);
	for (IndexT i = 0; i < newCapacity; i++)
		ASSERT(vec[i] == i);
}

template<typename VecT>
void VecTestResize()
{
	using IndexT = VecT::IndexType;
	VecT vec = VecT();

	vec.Resize(8, 0);
	ASSERT(!vec.IsEmpty());
	ASSERT(vec.Size() == 8);
	ASSERT(vec.Capacity() == 8);
	ASSERT(vec.Data() != nullptr);
	for (IndexT i = 0; i < 8; i++)
		ASSERT(vec[i] == 0);

	vec.Resize(0, 0); // Should free the memory
	ASSERT(vec.IsEmpty());
	ASSERT(vec.Size() == 0);
	ASSERT(vec.Capacity() == 0);
	ASSERT(vec.Data() == nullptr);

	vec.Reserve(16);
	auto ptr = vec.Data();
	vec.Resize(16, 0); // Resize when already at the right size should not allocate
	ASSERT(!vec.IsEmpty());
	ASSERT(vec.Size() == 16);
	ASSERT(vec.Capacity() == 16);
	ASSERT(vec.Data() == ptr);

	for (IndexT i = 0; i < 16; i++)
		vec[i] = (U32)i;

	vec.Resize(8, 0); // Smaller resize should clip the content but keep the start the same
	ASSERT(!vec.IsEmpty());
	ASSERT(vec.Size() == 8);
	ASSERT(vec.Capacity() == 8);

	for (IndexT i = 0; i < 8; i++)
		ASSERT(vec[i] == i);
}

template<typename VecT>
void VecTestFree()
{
	VecT vec = VecT();

	vec.Free();
	ASSERT(vec.IsEmpty());
	ASSERT(vec.Size() == 0);
	ASSERT(vec.Capacity() == 0);
	ASSERT(vec.Data() == nullptr);

	vec.Resize(8, 0);
	vec.Free();
	ASSERT(vec.IsEmpty());
	ASSERT(vec.Size() == 0);
	ASSERT(vec.Capacity() == 0);
	ASSERT(vec.Data() == nullptr);
}

template<typename VecT>
void VecBaseTestSubscript()
{
	using IndexT = VecT::IndexType;
	VecT vec = VecT();
	vec.Resize(8, 0);

	for (IndexT i = 0; i < 8; i++)
		vec[i] = (U32)i;

	for (IndexT i = 0; i < 8; i++)
		ASSERT(vec[i] == (U32)i);

	const VecT& ref = vec; // Make sure that the operator exists when const
	for (IndexT i = 0; i < 8; i++)
		ASSERT(ref[i] == i);
}

template<typename VecT>
void VecBaseTestIsEmpty()
{
	VecT vec = VecT();
	ASSERT(vec.Size() == 0);
	ASSERT(vec.IsEmpty());

	vec.Reserve(8);
	ASSERT(vec.Size() == 0);
	ASSERT(vec.IsEmpty());

	vec.Resize(8, 0);
	ASSERT(vec.Size() != 0);
	ASSERT(!vec.IsEmpty());
}

template<typename VecT>
void VecBaseTestFirstLast()
{
	using IndexT = VecT::IndexType;
	VecT vec = VecT();
	vec.Resize(8, 0);
	for (IndexT i = 0; i < 8; i++)
		vec[i] = (U32)i;

	ASSERT(vec.First() == 0);
	ASSERT(vec.Last() == 7);

	vec.First() = 1;
	vec.Last() = 6;

	const VecT& ref = vec;
	ASSERT(ref.First() == 1);
	ASSERT(ref.Last() == 6);
}

template<typename VecT>
void VecBaseTestClear()
{
	VecT vec = VecT();
	auto ptr = vec.Data();

	vec.Clear();
	ASSERT(vec.IsEmpty());
	ASSERT(vec.Size() == 0);
	ASSERT(vec.Data() == ptr);

	vec.Resize(8, 0);
	ptr = vec.Data();
	vec.Clear();
	ASSERT(vec.IsEmpty());
	ASSERT(vec.Size() == 0);
	ASSERT(vec.Capacity() >= 8);
	ASSERT(vec.Data() == ptr);
}

template<typename VecT>
void VecBaseTestPushBack()
{
	using IndexT = VecT::IndexType;
	VecT vec = VecT();
	ASSERT(vec.Size() == 0);

	vec.PushBack(1);
	ASSERT(vec.Size() == 1);
	ASSERT(vec[0] == 1);

	vec.Clear();

	IndexT maxSize = Math::MaxValue<IndexT>();
	if constexpr (requires { VecT::FixedSize; })
		maxSize = VecT::FixedSize;

	auto capacity = vec.Capacity();
	auto newSize = Math::Min<IndexT>(maxSize, capacity * 2);
	auto ptr = vec.Data();
	for (IndexT i = 0; i < newSize; i++)
		vec.PushBack((U32)i);

	ASSERT(vec.Capacity() >= newSize);
	if constexpr (requires { VecT::FixedSize; } == false)
		ASSERT(vec.Data() != ptr);
	ASSERT(vec.Size() == newSize);

	for (IndexT i = 0; i < newSize; i++)
		ASSERT(vec[i] == i);
}

template<typename VecT>
void VecBaseTestEmplaceBack()
{
	using IndexT = VecT::IndexType;
	VecT vec = VecT();
	ASSERT(vec.Size() == 0);

	vec.EmplaceBack(1);
	ASSERT(vec.Size() == 1);
	ASSERT(vec[0] == 1);

	vec.Clear();
	
	IndexT maxSize = Math::MaxValue<IndexT>();
	if constexpr (requires { VecT::FixedSize; })
		maxSize = VecT::FixedSize;
		
	auto capacity = vec.Capacity();
	auto newSize = Math::Min<IndexT>(maxSize, capacity * 2);
	auto ptr = vec.Data();
	for (IndexT i = 0; i < newSize; i++)
		vec.EmplaceBack((U32)i);

	ASSERT(vec.Capacity() >= newSize);
	if constexpr (requires { VecT::FixedSize; } == false)
		ASSERT(vec.Data() != ptr);
	ASSERT(vec.Size() == newSize);

	for (IndexT i = 0; i < newSize; i++)
		ASSERT(vec[i] == i);
}

template<typename VecT>
void VecBaseTestInsertAt()
{
	using IndexT = VecT::IndexType;
	VecT vec = VecT();
	for (U32 i = 0; i < 16; i++)
		vec.PushBack(i);

	vec.InsertAt(5, 100);
	ASSERT(vec.Size() == 17);
	ASSERT(vec[5] == 100);

	for (IndexT i = 0; i < 5; i++)
		ASSERT(vec[i] == i);

	for (IndexT i = 6; i < 17; i++)
		ASSERT(vec[i] == (U32)(i - 1));
}

template<typename VecT>
void VecBaseTestEmplaceAt()
{
	using IndexT = VecT::IndexType;
	VecT vec = VecT();
	for (U32 i = 0; i < 16; i++)
		vec.EmplaceBack(i);

	vec.EmplaceAt(5, 100);
	ASSERT(vec.Size() == 17);
	ASSERT(vec[5] == 100);

	for (IndexT i = 0; i < 5; i++)
		ASSERT(vec[i] == i);

	for (IndexT i = 6; i < 17; i++)
		ASSERT(vec[i] == (U32)(i - 1));
}

template<typename VecT>
void VecBaseTestPopBack()
{
	using IndexT = VecT::IndexType;
	VecT vec = VecT();
	for (U32 i = 0; i < 16; i++)
		vec.EmplaceBack(i);

	for (S32 i = 15; i >= 0; i--)
	{
		ASSERT(vec.PopBack() == (U32)i);
		ASSERT(vec.Size() == (IndexT)i);
	}
}

template<typename VecT>
void VecBaseTestRemoveAt()
{
	using IndexT = VecT::IndexType;
	VecT vec = VecT();
	for (U32 i = 0; i < 16; i++)
		vec.EmplaceBack(i);

	vec.RemoveAt(5);
	ASSERT(vec.Size() == 15);
	ASSERT(vec[5] == 15);

	for (IndexT i = 0; i < 5; i++)
		ASSERT(vec[i] == i);

	for (IndexT i = 6; i < 15; i++)
		ASSERT(vec[i] == i);
}

template<typename VecT>
void VecBaseTestRemoveAtOrdered()
{
	using IndexT = VecT::IndexType;
	VecT vec = VecT();
	for (U32 i = 0; i < 16; i++)
		vec.EmplaceBack(i);

	vec.RemoveAtOrdered(5);
	ASSERT(vec.Size() == 15);
	ASSERT(vec[5] == 6);

	for (IndexT i = 0; i < 5; i++)
		ASSERT(vec[i] == i);

	for (IndexT i = 6; i < 15; i++)
		ASSERT(vec[i] == (U32)(i + 1));
}

template<typename VecT>
void VecBaseTestForEach()
{
	using IndexT = VecT::IndexType;
	using ValueT = VecT::ValueType;
	VecT vec = VecT();
	for (U32 i = 0; i < 16; i++)
		vec.EmplaceBack(i);

	using CopyValueT = std::conditional_t<std::is_copy_assignable_v<ValueT>, ValueT, const ValueT&>;

	U32 index = 0;
	for (CopyValueT value : vec)
	{
		ASSERT(value == index);
		index++;
	}

	index = 0;
	for (ValueT& value : vec)
	{
		ASSERT(value == index);
		value = std::move(value * 2);
		index++;
	}

	index = 0;
	for (const ValueT& value : vec)
	{
		ASSERT(value == index * 2);
		index++;
	}
}

template<typename VecT>
void VecBaseTestContains()
{
	VecT vec = VecT();
	for (U32 i = 0; i < 16; i++)
		vec.EmplaceBack(i);

	for (U32 i = 0; i < 16; i++)
		ASSERT(vec.Contains(i));

	const VecT& ref = vec;
	ASSERT(ref.Contains(0));

	ASSERT(!vec.Contains(100));
}

template<typename VecT>
void VecBaseTestClone()
{
	using IndexT = VecT::IndexType;
	VecT vec = VecT();
	for (U32 i = 0; i < 16; i++)
		vec.EmplaceBack(i);

	const VecT& ref = vec;
	VecT clone = ref.Clone();

	ASSERT(vec.Size() == clone.Size());
	ASSERT(vec.Data() != clone.Data());

	for (IndexT i = 0; i < 16; i++)
	{
		ASSERT(vec[i] == clone[i]);
	}
}


template<typename VecT>
void TestVectorBase()
{
	VecBaseTestSubscript<VecT>();
	VecBaseTestIsEmpty<VecT>();
	VecBaseTestFirstLast<VecT>();
	VecBaseTestClear<VecT>();
	VecBaseTestEmplaceBack<VecT>();
	VecBaseTestEmplaceAt<VecT>();
	VecBaseTestPopBack<VecT>();
	VecBaseTestRemoveAt<VecT>();
	VecBaseTestRemoveAtOrdered<VecT>();
	VecBaseTestForEach<VecT>();
	VecBaseTestContains<VecT>();
	VecBaseTestClone<VecT>();
	VecBaseTestPushBack<VecT>();
	VecBaseTestInsertAt<VecT>();
}

template<typename VecT>
void TestVector()
{
	VecT vec = VecT();
	ASSERT(vec.IsEmpty());
	ASSERT(vec.Size() == 0);
	ASSERT(vec.Capacity() == 0);
	ASSERT(vec.Data() == nullptr);

	VecTestReserve<VecT>();
	VecTestResize<VecT>();
	VecTestFree<VecT>();
	
	TestVectorBase<VecT>();
}

TEST_CASE("Containers/Vector")
{
	TestVector<Vector<U32>>();
	TestVector<Vector<MoveOnlyType>>();

	TestVector<SmallVector<U32>>();
	TestVector<SmallVector<MoveOnlyType>>();

	TestVector<BigVector<U32>>();
	TestVector<BigVector<MoveOnlyType>>();
}

template<typename VecT>
void InplaceVecTestResize()
{
	using IndexT = VecT::IndexType;
	VecT vec = VecT();
	auto inplaceBuffer = vec.Data();

	vec.Resize(VecT::InplaceCapacity / 2, 0); // Smaller than the inplace buffer should stay inplace
	ASSERT(!vec.IsEmpty());
	ASSERT(vec.Size() == VecT::InplaceCapacity / 2);
	ASSERT(vec.Capacity() == VecT::InplaceCapacity);
	ASSERT(vec.Data() == inplaceBuffer);

	vec.Resize(64, 0);
	ASSERT(!vec.IsEmpty());
	ASSERT(vec.Size() == 64);
	ASSERT(vec.Capacity() == 64);
	ASSERT(vec.Data() != nullptr);
	ASSERT(vec.Data() != inplaceBuffer);
	for (IndexT i = 0; i < 64; i++)
		ASSERT(vec[i] == 0);

	vec.Resize(0, 0); // Should free the memory and go back to the inplace buffer
	ASSERT(vec.IsEmpty());
	ASSERT(vec.Size() == 0);
	ASSERT(vec.Capacity() == VecT::InplaceCapacity);
	ASSERT(vec.Data() == inplaceBuffer);

	vec.Reserve(128);
	auto ptr = vec.Data();
	vec.Resize(128, 0); // Resize when already at the right size should not allocate
	ASSERT(!vec.IsEmpty());
	ASSERT(vec.Size() == 128);
	ASSERT(vec.Capacity() == 128);
	ASSERT(vec.Data() == ptr);
	ASSERT(vec.Data() != inplaceBuffer);

	for (IndexT i = 0; i < VecT::InplaceCapacity; i++)
		vec[i] = (U32)i;

	vec.Resize(VecT::InplaceCapacity / 2, 0); // Should go back to the inplace buffer
	ASSERT(!vec.IsEmpty());
	ASSERT(vec.Size() == VecT::InplaceCapacity / 2);
	ASSERT(vec.Capacity() == VecT::InplaceCapacity);
	ASSERT(vec.Data() == inplaceBuffer);

	for (IndexT i = 0; i < VecT::InplaceCapacity / 2; i++)
		ASSERT(vec[i] == i);
}

template<typename VecT>
void InplaceVecTestFree()
{
	VecT vec = VecT();
	auto inplaceBuffer = vec.Data();

	vec.Free();
	ASSERT(vec.IsEmpty());
	ASSERT(vec.Size() == 0);
	ASSERT(vec.Capacity() == VecT::InplaceCapacity);
	ASSERT(vec.Data() == inplaceBuffer);

	vec.Resize(VecT::InplaceCapacity * 2, 0);
	vec.Free();
	ASSERT(vec.IsEmpty());
	ASSERT(vec.Size() == 0);
	ASSERT(vec.Capacity() == VecT::InplaceCapacity);
	ASSERT(vec.Data() == inplaceBuffer);
}

template<typename VecT>
void TestInplaceVector()
{
	VecT vec = VecT();
	ASSERT(vec.IsEmpty());
	ASSERT(vec.Size() == 0);
	ASSERT(vec.Capacity() == VecT::InplaceCapacity);
	ASSERT(vec.Data() != nullptr);

	VecTestReserve<VecT>();
	InplaceVecTestResize<VecT>();
	InplaceVecTestFree<VecT>();

	TestVectorBase<VecT>();
}

TEST_CASE("Containers/InplaceVector")
{
	TestInplaceVector<InplaceVector<U32, 16>>();
	TestInplaceVector<InplaceVector<MoveOnlyType, 16>>();

	TestInplaceVector<SmallInplaceVector<U32, 16>>();
	TestInplaceVector<SmallInplaceVector<MoveOnlyType, 16>>();

	TestInplaceVector<BigInplaceVector<U32, 16>>();
	TestInplaceVector<BigInplaceVector<MoveOnlyType, 16>>();
}

template<typename VecT>
void FixedVecTestResize()
{
	using IndexT = VecT::IndexType;
	VecT vec = VecT();
	auto inplaceBuffer = vec.Data();

	vec.Resize(VecT::FixedSize / 2, 0); // Smaller than the inplace buffer should stay inplace
	ASSERT(!vec.IsEmpty());
	ASSERT(vec.Size() == VecT::FixedSize / 2);
	ASSERT(vec.Capacity() == VecT::FixedSize);
	ASSERT(vec.Data() == inplaceBuffer);

	vec.Resize(0, 0); // Should free the memory and go back to the inplace buffer
	ASSERT(vec.IsEmpty());
	ASSERT(vec.Size() == 0);
	ASSERT(vec.Capacity() == VecT::FixedSize);
	ASSERT(vec.Data() == inplaceBuffer);

	vec.Resize(VecT::FixedSize, 0);
	ASSERT(!vec.IsEmpty());
	ASSERT(vec.Size() == VecT::FixedSize);
	ASSERT(vec.Capacity() == VecT::FixedSize);
	ASSERT(vec.Data() == inplaceBuffer);

	for (IndexT i = 0; i < VecT::FixedSize; i++)
		vec[i] = (U32)i;

	vec.Resize(VecT::FixedSize / 2, 0);
	ASSERT(!vec.IsEmpty());
	ASSERT(vec.Size() == VecT::FixedSize / 2);
	ASSERT(vec.Capacity() == VecT::FixedSize);
	ASSERT(vec.Data() == inplaceBuffer);

	for (IndexT i = 0; i < VecT::FixedSize / 2; i++)
		ASSERT(vec[i] == i);
}

template<typename VecT>
void TestFixedVector()
{
	using IndexT = VecT::IndexType;
	VecT vec = VecT();
	ASSERT(vec.IsEmpty());
	ASSERT(vec.Size() == 0);
	ASSERT(vec.Capacity() == VecT::FixedSize);
	ASSERT(vec.Data() != nullptr);

	// Reserve
	vec.Reserve(VecT::FixedSize / 2); // Should do nothing
	ASSERT(vec.IsEmpty());
	ASSERT(vec.Size() == 0);
	ASSERT(vec.Capacity() == VecT::FixedSize);
	ASSERT(vec.Data() != nullptr);

	vec.Reserve(VecT::FixedSize); // Should do nothing
	ASSERT(vec.IsEmpty());
	ASSERT(vec.Size() == 0);
	ASSERT(vec.Capacity() == VecT::FixedSize);
	ASSERT(vec.Data() != nullptr);

	// Free
	auto ptr = vec.Data();
	vec.Free();
	ASSERT(vec.IsEmpty());
	ASSERT(vec.Size() == 0);
	ASSERT(vec.Capacity() == VecT::FixedSize);
	ASSERT(vec.Data() == ptr);

	FixedVecTestResize<VecT>();
	TestVectorBase<VecT>();
}

TEST_CASE("Containers/FixedVector")
{
	TestFixedVector<FixedVector<U32, 32>>();
	TestFixedVector<FixedVector<MoveOnlyType, 32>>();

	TestFixedVector<SmallFixedVector<U32, 32>>();
	TestFixedVector<SmallFixedVector<MoveOnlyType, 32>>();

	TestFixedVector<BigFixedVector<U32, 32>>();
	TestFixedVector<BigFixedVector<MoveOnlyType, 32>>();
}
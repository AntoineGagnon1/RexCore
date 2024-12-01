#include <tests/test_utils.hpp>

#include <rexcore/containers/vector.hpp>
#include <rexcore/containers/string.hpp>
#include <rexcore/containers/map.hpp>
#include <rexcore/containers/set.hpp>
#include <rexcore/containers/smart_ptrs.hpp>
#include <rexcore/containers/function.hpp>
#include <rexcore/containers/deque.hpp>
#include <rexcore/containers/stack.hpp>
#include <rexcore/containers/ring_buffer.hpp>
#include <rexcore/containers/no_destructor.hpp>
#include <rexcore/math.hpp>
#include <rexcore/time.hpp>

#include <thread>

using namespace RexCore;

class MoveOnlyType
{
public:
	MoveOnlyType(MoveOnlyType&& other) = default;
	MoveOnlyType& operator=(MoveOnlyType&& other) = default;

	MoveOnlyType(const MoveOnlyType&) = delete;
	MoveOnlyType& operator=(const MoveOnlyType&) = delete;

	explicit MoveOnlyType(U16 value) : value((U32)value) {}
	explicit MoveOnlyType(U32 value) : value((U32)value) {}
	explicit MoveOnlyType(U64 value) : value((U32)value) {}
	explicit MoveOnlyType(int value) : value((U32)value) {}

	explicit operator U32() const { return value; }

	MoveOnlyType Clone() const
	{
		return MoveOnlyType(value);
	}

	auto operator<=>(const MoveOnlyType&) const = default;

	U32 value;
};

// The span must be filled with : {0, 1, 2, 3, 4, 5, 6, ..., 15} 
template<typename SpanT>
void TestSpanTypeBase(SpanT span)
{
	using ValueT = SpanT::ValueType;
	using IndexT = SpanT::IndexType;

	{ // Operator []
		for (IndexT i = 0; i < 16; i++)
			ASSERT(span[i] == ValueT(i));
	}
	
	{ // IsEmpty
		ASSERT(!span.IsEmpty());
		SpanT span2;
		ASSERT(span2.IsEmpty());
	}

	{ // First and Last
		ASSERT(span.First() == ValueT(0));
		ASSERT(span.Last() == ValueT(15));
	}

	{ // Begin and End
		ASSERT(span.Begin() == span.Data());
		ASSERT(span.CBegin() == span.Data());
		ASSERT(span.End() == span.Data() + span.Size());
		ASSERT(span.CEnd() == span.Data() + span.Size());
		
		ASSERT(*span.Begin() == ValueT(0));
		ASSERT(*span.CBegin() == ValueT(0));
		ASSERT(*(span.End() - 1) == ValueT(15));
		ASSERT(*(span.CEnd() - 1) == ValueT(15));
	}

	{ // Contains
		for (IndexT i = 0; i < 16; i++) {
			ASSERT(span.Contains(ValueT(i)));
			ASSERT(span.Contains([i](const ValueT& v) {return v == ValueT(i); }));
		}

		ASSERT(!span.Contains(ValueT(100)));
		ASSERT(!span.Contains([](const ValueT& v) {return v == ValueT(100); }));
	}


	{ // TryFind
		for (IndexT i = 0; i < 16; i++)
		{
			ASSERT(*span.TryFind(ValueT(i)) == ValueT(i));
			ASSERT(*span.TryFind([i](const ValueT& v) { return v == ValueT(i); }) == ValueT(i));
		}

		ASSERT(span.TryFind(ValueT(100)) == nullptr);
		ASSERT(span.TryFind([](const ValueT& v) { return v == ValueT(100); }) == nullptr);
	}

	{ // IndexOf
		for (IndexT i = 0; i < 16; i++)
		{
			ASSERT(span.IndexOf(ValueT(i)) == i);
		}

		ASSERT(span.IndexOf(ValueT(100)) == span.Size());
	}

	{ // SubSpan
		const SpanT subSpan = span.SubSpan(3, 5);
		ASSERT(subSpan.Size() == 5);
		for (IndexT i = 0; i < 5; i++)
			ASSERT(subSpan[i] == ValueT(i + 3));

		const SpanT emptySpan = span.SubSpan(16, 5);
		ASSERT(emptySpan.IsEmpty());

		const SpanT overshoot = span.SubSpan(14, 5);
		ASSERT(overshoot.Size() == 2);
		for (IndexT i = 0; i < 2; i++)
			ASSERT(overshoot[i] == ValueT(i + 14));

		const SpanT oneArg = span.SubSpan(2);
		ASSERT(oneArg.Size() == 14);
		for (IndexT i = 0; i < 14; i++)
			ASSERT(oneArg[i] == ValueT(i + 2));
	}
}

TEST_CASE("Containers/Span")
{
	{ // Empty span
		Span<U32> span = Span<U32>();
		ASSERT(span.Data() == nullptr);
		ASSERT(span.Size() == 0);
		ASSERT(span.IsEmpty());
	}
	{ // Data() and Size()
		Vector<U32> vec = Vector<U32>();
		for (U32 i = 0; i < 16; i++)
			vec.EmplaceBack(i);

		Span<U32> span = Span(vec.Data(), vec.Size());
		ASSERT(span.Data() == vec.Data());
		ASSERT(span.Size() == vec.Size());

		for (U32 i = 0; i < 16; i++)
			ASSERT(span[i] == i);
	}

	{ // Foreach
		Vector<U32> vec = Vector<U32>();
		vec.Resize(16, 0);
		Span span = Span(vec.Data(), vec.Size());
		for (const U32& value : span)
			ASSERT(value == 0);
	}

	{
		Vector<U32> vec = Vector<U32>();
		for (U32 i = 0; i < 16; i++)
			vec.EmplaceBack(i);
		TestSpanTypeBase(Span(vec.Data(), vec.Size()));
	}
}

template<typename VecT, typename ...Args>
void VecTestReserve(Args&& ...args)
{
	using IndexT = VecT::IndexType;
	using ValueT = VecT::ValueType;
	VecT vec = VecT(args...);

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
		vec.EmplaceBack(ValueT(i));

	vec.Reserve(vec.Capacity() * 2); // Make sure that reserving does not change the content
	for (IndexT i = 0; i < newCapacity; i++)
		ASSERT(vec[i] == ValueT(i));
}

template<typename VecT, typename ...Args>
void VecTestResize(Args&& ...args)
{
	using IndexT = VecT::IndexType;
	using ValueT = VecT::ValueType;
	VecT vec = VecT(args...);

	vec.Resize(8, 0);
	ASSERT(!vec.IsEmpty());
	ASSERT(vec.Size() == 8);
	ASSERT(vec.Capacity() == 8);
	ASSERT(vec.Data() != nullptr);
	for (IndexT i = 0; i < 8; i++)
		ASSERT(vec[i] == ValueT(0));

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
		vec[i] = ValueT(i);

	vec.Resize(8, 0); // Smaller resize should clip the content but keep the start the same
	ASSERT(!vec.IsEmpty());
	ASSERT(vec.Size() == 8);
	ASSERT(vec.Capacity() == 8);

	for (IndexT i = 0; i < 8; i++)
		ASSERT(vec[i] == ValueT(i));
}

template<typename VecT, typename ...Args>
void VecTestFree(Args&& ...args)
{
	VecT vec = VecT(args...);

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

template<typename VecT, typename ...Args>
void VecBaseTestSubscript(Args&& ...args)
{
	using IndexT = VecT::IndexType;
	using ValueT = VecT::ValueType;
	VecT vec = VecT(args...);
	vec.Resize(8, 0);

	for (IndexT i = 0; i < 8; i++)
		vec[i] = (ValueT)i;

	for (IndexT i = 0; i < 8; i++)
		ASSERT(vec[i] == (ValueT)i);

	const VecT& ref = vec; // Make sure that the operator exists when const
	for (IndexT i = 0; i < 8; i++)
		ASSERT(ref[i] == ValueT(i));
}

template<typename VecT, typename ...Args>
void VecBaseTestIsEmpty(Args&& ...args)
{
	VecT vec = VecT(args...);
	ASSERT(vec.Size() == 0);
	ASSERT(vec.IsEmpty());

	vec.Reserve(8);
	ASSERT(vec.Size() == 0);
	ASSERT(vec.IsEmpty());

	vec.Resize(8, 0);
	ASSERT(vec.Size() != 0);
	ASSERT(!vec.IsEmpty());
}

template<typename VecT, typename ...Args>
void VecBaseTestFirstLast(Args&& ...args)
{
	using IndexT = VecT::IndexType;
	using ValueT = VecT::ValueType;
	VecT vec = VecT(args...);
	vec.Resize(8, 0);
	for (IndexT i = 0; i < 8; i++)
		vec[i] = ValueT(i);

	ASSERT(vec.First() == ValueT(0));
	ASSERT(vec.Last() == ValueT(7));

	vec.First() = ValueT(1);
	vec.Last() = ValueT(6);

	const VecT& ref = vec;
	ASSERT(ref.First() == ValueT(1));
	ASSERT(ref.Last() == ValueT(6));
}

template<typename VecT, typename ...Args>
void VecBaseTestClear(Args&& ...args)
{
	VecT vec = VecT(args...);
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

template<typename VecT, typename ...Args>
void VecBaseTestPushBack(Args&& ...args)
{
	using IndexT = VecT::IndexType;
	using ValueT = VecT::ValueType;
	VecT vec = VecT(args...);
	ASSERT(vec.Size() == 0);

	vec.PushBack(ValueT(1));
	ASSERT(vec.Size() == 1);
	ASSERT(vec[0] == ValueT(1));

	vec.Clear();

	IndexT maxSize = Math::MaxValue<IndexT>();
	if constexpr (requires { VecT::FixedSize; })
		maxSize = VecT::FixedSize;

	auto capacity = vec.Capacity();
	auto newSize = Math::Min<IndexT>(maxSize, capacity * 2);
	auto ptr = vec.Data();
	for (IndexT i = 0; i < newSize; i++)
		vec.PushBack(ValueT(i));

	ASSERT(vec.Capacity() >= newSize);
	if constexpr (requires { VecT::FixedSize; } == false)
		ASSERT(vec.Data() != ptr);
	ASSERT(vec.Size() == newSize);

	for (IndexT i = 0; i < newSize; i++)
		ASSERT(vec[i] == ValueT(i));
}

template<typename VecT, typename ...Args>
void VecBaseTestEmplaceBack(Args&& ...args)
{
	using IndexT = VecT::IndexType;
	using ValueT = VecT::ValueType;
	VecT vec = VecT(args...);
	ASSERT(vec.Size() == 0);

	vec.EmplaceBack(ValueT(1));
	ASSERT(vec.Size() == 1);
	ASSERT(vec[0] == ValueT(1));

	vec.Clear();
	
	IndexT maxSize = Math::MaxValue<IndexT>();
	if constexpr (requires { VecT::FixedSize; })
		maxSize = VecT::FixedSize;
		
	auto capacity = vec.Capacity();
	auto newSize = Math::Min<IndexT>(maxSize, capacity * 2);
	auto ptr = vec.Data();
	for (IndexT i = 0; i < newSize; i++)
		vec.EmplaceBack(ValueT(i));

	ASSERT(vec.Capacity() >= newSize);
	if constexpr (requires { VecT::FixedSize; } == false)
		ASSERT(vec.Data() != ptr);
	ASSERT(vec.Size() == newSize);

	for (IndexT i = 0; i < newSize; i++)
		ASSERT(vec[i] == ValueT(i));
}

template<typename VecT, typename ...Args>
void VecBaseTestInsertAt(Args&& ...args)
{
	using IndexT = VecT::IndexType;
	using ValueT = VecT::ValueType;
	VecT vec = VecT(args...);
	for (U32 i = 0; i < 16; i++)
		vec.PushBack(ValueT(i));

	vec.InsertAt(5, ValueT(100));
	ASSERT(vec.Size() == 17);
	ASSERT(vec[5] == ValueT(100));

	for (IndexT i = 0; i < 5; i++)
		ASSERT(vec[i] == ValueT(i));

	for (IndexT i = 6; i < 17; i++)
		ASSERT(vec[i] == ValueT(i - 1));
}

template<typename VecT, typename ...Args>
void VecBaseTestEmplaceAt(Args&& ...args)
{
	using IndexT = VecT::IndexType;
	using ValueT = VecT::ValueType;
	VecT vec = VecT(args...);
	for (U32 i = 0; i < 16; i++)
		vec.EmplaceBack(ValueT(i));

	vec.EmplaceAt(5, ValueT(100));
	ASSERT(vec.Size() == 17);
	ASSERT(vec[5] == ValueT(100));

	for (IndexT i = 0; i < 5; i++)
		ASSERT(vec[i] == ValueT(i));

	for (IndexT i = 6; i < 17; i++)
		ASSERT(vec[i] == ValueT(i - 1));
}

template<typename VecT, typename ...Args>
void VecBaseTestPopBack(Args&& ...args)
{
	using IndexT = VecT::IndexType;
	using ValueT = VecT::ValueType;
	VecT vec = VecT(args...);
	for (U32 i = 0; i < 16; i++)
		vec.EmplaceBack(ValueT(i));

	for (S32 i = 15; i >= 0; i--)
	{
		ASSERT(vec.PopBack() == ValueT(i));
		ASSERT(vec.Size() == (IndexT)i);
	}
}

template<typename VecT, typename ...Args>
void VecBaseTestRemoveAt(Args&& ...args)
{
	using IndexT = VecT::IndexType;
	using ValueT = VecT::ValueType;
	VecT vec = VecT(args...);
	for (U32 i = 0; i < 16; i++)
		vec.EmplaceBack(ValueT(i));

	vec.RemoveAt(5);
	ASSERT(vec.Size() == 15);
	ASSERT(vec[5] == ValueT(15));

	for (IndexT i = 0; i < 5; i++)
		ASSERT(vec[i] == ValueT(i));

	for (IndexT i = 6; i < 15; i++)
		ASSERT(vec[i] == ValueT(i));
}

template<typename VecT, typename ...Args>
void VecBaseTestRemoveAtOrdered(Args&& ...args)
{
	using IndexT = VecT::IndexType;
	using ValueT = VecT::ValueType;
	VecT vec = VecT(args...);
	for (U32 i = 0; i < 16; i++)
		vec.EmplaceBack(ValueT(i));

	vec.RemoveAtOrdered(5);
	ASSERT(vec.Size() == 15);
	ASSERT(vec[5] == ValueT(6));

	for (IndexT i = 0; i < 5; i++)
		ASSERT(vec[i] == ValueT(i));

	for (IndexT i = 6; i < 15; i++)
		ASSERT(vec[i] == ValueT(i + 1));
}

template<typename VecT, typename ...Args>
void VecBaseTestRemove(Args&& ...args)
{
	using IndexT = VecT::IndexType;
	using ValueT = VecT::ValueType;
	VecT vec = VecT(args...);
	for (U32 i = 0; i < 16; i++)
		vec.EmplaceBack(ValueT(i));

	vec.Remove(ValueT(5));
	ASSERT(vec.Size() == 15);
	ASSERT(vec[5] == ValueT(15));

	for (IndexT i = 0; i < 5; i++)
		ASSERT(vec[i] == ValueT(i));

	for (IndexT i = 6; i < 15; i++)
		ASSERT(vec[i] == ValueT(i));
}

template<typename VecT, typename ...Args>
void VecBaseTestRemoveOrdered(Args&& ...args)
{
	using IndexT = VecT::IndexType;
	using ValueT = VecT::ValueType;
	VecT vec = VecT(args...);
	for (U32 i = 0; i < 16; i++)
		vec.EmplaceBack(ValueT(i));

	vec.RemoveOrdered(ValueT(5));
	ASSERT(vec.Size() == 15);
	ASSERT(vec[5] == ValueT(6));

	for (IndexT i = 0; i < 5; i++)
		ASSERT(vec[i] == ValueT(i));

	for (IndexT i = 6; i < 15; i++)
		ASSERT(vec[i] == ValueT(i + 1));
}

template<typename VecT, typename ...Args>
void VecBaseTestForEach(Args&& ...args)
{
	using IndexT = VecT::IndexType;
	using ValueT = VecT::ValueType;
	VecT vec = VecT(args...);
	for (U32 i = 0; i < 16; i++)
		vec.EmplaceBack(ValueT(i));

	using CopyValueT = std::conditional_t<std::is_copy_assignable_v<ValueT>, ValueT, const ValueT&>;

	U32 index = 0;
	for (CopyValueT value : vec)
	{
		ASSERT(value == ValueT(index));
		index++;
	}

	index = 0;
	for (ValueT& value : vec)
	{
		ASSERT(value == ValueT(index));
		value = ValueT(U32(value) * 2);
		index++;
	}

	index = 0;
	for (const ValueT& value : vec)
	{
		ASSERT(value == ValueT(index * 2));
		index++;
	}
}

template<typename VecT, typename ...Args>
void VecBaseTestContains(Args&& ...args)
{
	using ValueT = VecT::ValueType;

	VecT vec = VecT(args...);
	for (U32 i = 0; i < 16; i++)
		vec.EmplaceBack(ValueT(i));

	for (U32 i = 0; i < 16; i++)
		ASSERT(vec.Contains(ValueT(i)));

	const VecT& ref = vec;
	ASSERT(ref.Contains(ValueT(0)));

	ASSERT(!vec.Contains(ValueT(100)));
}

template<typename VecT, typename ...Args>
void VecBaseTestClone(Args&& ...args)
{
	using IndexT = VecT::IndexType;
	using ValueT = VecT::ValueType;

	VecT vec = VecT(args...);
	for (U32 i = 0; i < 16; i++)
		vec.EmplaceBack(ValueT(i));

	const VecT& ref = vec;
	VecT clone = ref.Clone();

	ASSERT(vec.Size() == clone.Size());
	ASSERT(vec.Data() != clone.Data());

	for (IndexT i = 0; i < 16; i++)
	{
		ASSERT(vec[i] == clone[i]);
	}
}


template<typename VecT, typename ...Args>
void TestVectorBase(Args&& ...args)
{
	VecBaseTestSubscript<VecT>(args...);
	VecBaseTestIsEmpty<VecT>(args...);
	VecBaseTestFirstLast<VecT>(args...);
	VecBaseTestClear<VecT>(args...);
	VecBaseTestEmplaceBack<VecT>(args...);
	VecBaseTestEmplaceAt<VecT>(args...);
	VecBaseTestPopBack<VecT>(args...);
	VecBaseTestRemoveAt<VecT>(args...);
	VecBaseTestRemoveAtOrdered<VecT>(args...);
	VecBaseTestRemove<VecT>(args...);
	VecBaseTestRemoveOrdered<VecT>(args...);
	VecBaseTestForEach<VecT>(args...);
	VecBaseTestContains<VecT>(args...);
	VecBaseTestClone<VecT>(args...);
	VecBaseTestPushBack<VecT>(args...);
	VecBaseTestInsertAt<VecT>(args...);

	{ // To and from span
		using SpanT = VecT::SpanType;
		using ValueT = VecT::ValueType;

		VecT vec = VecT(args...);
		vec.Resize(16, ValueT(4));
		SpanT span = vec;
		VecT vec2(span, args...);
		ASSERT(vec2.Size() == 16);
		ASSERT(vec.Data() != vec2.Data());
	}

	{ // Span functions
		using ValueT = VecT::ValueType;
		using IndexT = VecT::IndexType;
		using SpanT = VecT::SpanType;

		VecT vec = VecT(args...);
		for (IndexT i = 0; i < 16; i++)
			vec.EmplaceBack(ValueT(i));

		*vec.TryFind(ValueT(4)) = ValueT(4); // Make sure that TryFind is not const on Vectors

		const VecT& ref = vec;
		[[maybe_unused]] auto tmp = ref.TryFind(ValueT(4)); // Make sure that the const version is still available on vectors

		TestSpanTypeBase(SpanT(vec.Data(), vec.Size()));
	}
}

template<typename VecT, typename ...Args>
void TestVector(Args&& ...args)
{
	VecT vec = VecT(args...);
	ASSERT(vec.IsEmpty());
	ASSERT(vec.Size() == 0);
	ASSERT(vec.Capacity() == 0);
	ASSERT(vec.Data() == nullptr);

	VecTestReserve<VecT>(args...);
	VecTestResize<VecT>(args...);
	VecTestFree<VecT>(args...);
	
	TestVectorBase<VecT>(args...);
}

TEST_CASE("Containers/Vector")
{
	ArenaAllocator arena;

	TestVector<Vector<U32>>();
	TestVector<Vector<MoveOnlyType>>();
	TestVector<Vector<U32, ArenaAllocator>>(arena);

	TestVector<SmallVector<U32>>();
	TestVector<SmallVector<MoveOnlyType>>();
	TestVector<SmallVector<U32, ArenaAllocator>>(arena);

	TestVector<BigVector<U32>>();
	TestVector<BigVector<MoveOnlyType>>();
	TestVector<BigVector<U32, ArenaAllocator>>(arena);
	
}

template<typename VecT, typename ...Args>
void InplaceVecTestResize(Args&& ...args)
{
	using IndexT = VecT::IndexType;
	using ValueT = VecT::ValueType;
	VecT vec = VecT(args...);
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
		ASSERT(vec[i] == ValueT(0));

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
		vec[i] = ValueT(i);

	vec.Resize(VecT::InplaceCapacity / 2, 0); // Should go back to the inplace buffer
	ASSERT(!vec.IsEmpty());
	ASSERT(vec.Size() == VecT::InplaceCapacity / 2);
	ASSERT(vec.Capacity() == VecT::InplaceCapacity);
	ASSERT(vec.Data() == inplaceBuffer);

	for (IndexT i = 0; i < VecT::InplaceCapacity / 2; i++)
		ASSERT(vec[i] == ValueT(i));
}

template<typename VecT, typename ...Args>
void InplaceVecTestFree(Args&& ...args)
{
	VecT vec = VecT(args...);
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

template<typename VecT, typename ...Args>
void TestInplaceVector(Args&& ...args)
{
	VecT vec = VecT(args...);
	ASSERT(vec.IsEmpty());
	ASSERT(vec.Size() == 0);
	ASSERT(vec.Capacity() == VecT::InplaceCapacity);
	ASSERT(vec.Data() != nullptr);

	VecTestReserve<VecT>(args...);
	InplaceVecTestResize<VecT>(args...);
	InplaceVecTestFree<VecT>(args...);

	TestVectorBase<VecT>(args...);
}

TEST_CASE("Containers/InplaceVector")
{
	ArenaAllocator arena;
	TestInplaceVector<InplaceVector<U32, 16>>();
	TestInplaceVector<InplaceVector<MoveOnlyType, 16>>();
	TestInplaceVector<InplaceVector<U32, 16, ArenaAllocator>>(arena);

	TestInplaceVector<SmallInplaceVector<U32, 16>>();
	TestInplaceVector<SmallInplaceVector<MoveOnlyType, 16>>();
	TestInplaceVector<SmallInplaceVector<U32, 16, ArenaAllocator>>(arena);

	TestInplaceVector<BigInplaceVector<U32, 16>>();
	TestInplaceVector<BigInplaceVector<MoveOnlyType, 16>>();
	TestInplaceVector<BigInplaceVector<U32, 16, ArenaAllocator>>(arena);
}

template<typename VecT>
void FixedVecTestResize()
{
	using IndexT = VecT::IndexType;
	using ValueT = VecT::ValueType;
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
		vec[i] = ValueT(i);

	vec.Resize(VecT::FixedSize / 2, 0);
	ASSERT(!vec.IsEmpty());
	ASSERT(vec.Size() == VecT::FixedSize / 2);
	ASSERT(vec.Capacity() == VecT::FixedSize);
	ASSERT(vec.Data() == inplaceBuffer);

	for (IndexT i = 0; i < VecT::FixedSize / 2; i++)
		ASSERT(vec[i] == ValueT(i));
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


// view must be filled with : {0, 1, 2, 3, 4, 5, 6, ..., 15}
template<typename ViewT, typename ...Args>
void TestStringTypeBase(const ViewT& view, Args&& ...args)
{
	using CharT = ViewT::CharType;

	{ // SubStr
		const auto subStr = view.SubStr(3, 5);
		ASSERT(subStr.Size() == 5);
		for (U64 i = 0; i < 5; i++)
			ASSERT(subStr[i] == CharT(i + 3));

		const auto emptyView = view.SubStr(16, 5);
		ASSERT(emptyView.IsEmpty());

		const auto overshoot = view.SubStr(14, 5);
		ASSERT(overshoot.Size() == 2);
		for (U64 i = 0; i < 2; i++)
			ASSERT(overshoot[i] == CharT(i + 14));

		const auto oneArg = view.SubStr(2);
		ASSERT(oneArg.Size() == 14);
		for (U64 i = 0; i < 14; i++)
			ASSERT(oneArg[i] == CharT(i + 2));
	}

	{ // StartsWith, EndsWith
		if constexpr (std::is_same_v<CharT, char>)
		{
			ViewT v("abcdef123456", args...);
			ViewT v2("abc", args...);
			ASSERT(v.StartsWith(v2));
			ASSERT(v.StartsWith("a"));
			ASSERT(!v.StartsWith("bcdef"));

			ASSERT(v.EndsWith("456"));
			ASSERT(v.EndsWith("6"));
			ASSERT(!v.EndsWith("1234567"));
		}
		else
		{
			ViewT v(L"abcdef123456", args...);
			ViewT v2(L"abc", args...);
			ASSERT(v.StartsWith(v2));
			ASSERT(v.StartsWith(L"a"));
			ASSERT(!v.StartsWith(L"bcdef"));

			ASSERT(v.EndsWith(L"456"));
			ASSERT(v.EndsWith(L"6"));
			ASSERT(!v.EndsWith(L"1234567"));
		}
	}

	{ // SplitInto
		if constexpr (std::is_same_v<CharT, char>)
		{
			ViewT v("a,,b,c,", args...);
			InplaceVector<StringView, 4> split;
			v.SplitInto(split, ',');
			ASSERT(split.Size() == 4);
			ASSERT(split[0] == "a");
			ASSERT(split[1] == "");
			ASSERT(split[2] == "b");
			ASSERT(split[3] == "c");
		}
		else
		{
			ViewT v(L"a,,b,c,", args...);
			InplaceVector<WStringView, 4> split;
			v.SplitInto(split, ',');
			ASSERT(split.Size() == 4);
			ASSERT(split[0] == L"a");
			ASSERT(split[1] == L"");
			ASSERT(split[2] == L"b");
			ASSERT(split[3] == L"c");
		}

		if constexpr (std::is_same_v<CharT, char>)
		{
			ViewT v("abc", args...);
			InplaceVector<StringView, 1> split;
			v.SplitInto(split, ',');
			ASSERT(split.Size() == 1);
			ASSERT(split[0] == "abc");
		}
		else
		{
			ViewT v(L"abc", args...);
			InplaceVector<WStringView, 1> split;
			v.SplitInto(split, L',');
			ASSERT(split.Size() == 1);
			ASSERT(split[0] == L"abc");
		}

		if constexpr (std::is_same_v<CharT, char>)
		{
			ViewT v("abcsplitdefsplit", args...);
			InplaceVector<StringView, 2> split;
			v.SplitInto(split, "split");
			ASSERT(split.Size() == 2);
			ASSERT(split[0] == "abc");
			ASSERT(split[1] == "def");
		}
		else
		{
			ViewT v(L"abcsplitdefsplit", args...);
			InplaceVector<WStringView, 2> split;
			v.SplitInto(split, L"split");
			ASSERT(split.Size() == 2);
			ASSERT(split[0] == L"abc");
			ASSERT(split[1] == L"def");
		}
	}

	// ==, !=
	{
		ASSERT(view == view);
		auto strView = view.SubStr(0);
		ASSERT(view == strView);

		auto subStr = view.SubStr(0, 5);
		ASSERT(view != subStr);

		auto empty1 = ViewT(args...);
		auto empty2 = ViewT(args...);
		ASSERT(empty1 == empty2);

		if constexpr (std::is_same_v<CharT, char>)
		{
			ASSERT(empty1 == "");
			ViewT v("This is a string view", args...);
			ASSERT(v == "This is a string view");
			ASSERT(v != "aaaaaaaaaaaaaaaaaaaaa");
			ASSERT(v != "This is a different string view");
			ASSERT(v != empty1);
		}
		else
		{
			ASSERT(empty1 == L"");

			ViewT v(L"This is a string view", args...);
			ASSERT(v == L"This is a string view");
			ASSERT(v != L"aaaaaaaaaaaaaaaaaaaaa");
			ASSERT(v != L"This is a different string view");
			ASSERT(v != empty1);
		}
	}

	// <, <=, >, >=
	{
		if constexpr (std::is_same_v<CharT, char>)
		{
			ViewT v("abcdefg", args...);
			ViewT v2("abcg", args...);
			auto empty = ViewT(args...);

			ASSERT(v < v2);
			ASSERT(v <= v2);
			ASSERT(v2 > v);
			ASSERT(v2 >= v);
			ASSERT(v > empty);
			ASSERT(v >= empty);
			ASSERT(empty < v);
			ASSERT(empty <= v);
		}
		else
		{
			ViewT v(L"abcdefg", args...);
			ViewT v2(L"abcg", args...);
			auto empty = ViewT(args...);

			ASSERT(v < v2);
			ASSERT(v <= v2);
			ASSERT(v2 > v);
			ASSERT(v2 >= v);
			ASSERT(v > empty);
			ASSERT(v >= empty);
			ASSERT(empty < v);
			ASSERT(empty <= v);
		}
	}
}

template<typename StringT, typename ...Args>
void TestStringView(Args&& ...args)
{
	using StringViewT = typename StringT::StringViewType;
	using CharT = typename StringT::CharType;

	{ // Empty view
		StringViewT view;
		ASSERT(view.Data() == nullptr);
		ASSERT(view.Size() == 0);
		ASSERT(view.IsEmpty());
	}
	{ // Data() and Size()
		StringT str = StringT(args...);
		for (CharT i = 0; i < 16; i++)
			str.EmplaceBack(i);

		StringViewT view = str;
		ASSERT(view.Data() == str.Data());
		ASSERT(view.Size() == str.Size());

		for (U32 i = 0; i < 16; i++)
			ASSERT(view[i] == CharT(i));
	}

	{ // Foreach
		StringT str = StringT(args...);
		str.Resize(16, CharT('a'));
		StringViewT view = str;
		for (const CharT& value : view)
			ASSERT(value == CharT('a'));
	}

	{ // StringTypeBase functions
		StringT str;
		for (CharT i = 0; i < 16; i++)
			str.EmplaceBack(i);

		TestStringTypeBase<StringViewT>(str, args...);
	}

	{ // SpanBase functions
		StringT str;
		for (CharT i = 0; i < 16; i++)
			str.EmplaceBack(i);

		TestSpanTypeBase<StringViewT>(str);
	}
}

TEST_CASE("Containers/StringView")
{
	[[maybe_unused]] StringView view = "Hello 123"; // Implicit conversion from c-style string
	TestStringView<String<>>();
}

TEST_CASE("Containers/WStringView")
{
	[[maybe_unused]] WStringView view = L"Hello 123"; // Implicit conversion from c-style string
	TestStringView<WString<>>();
}

template<typename StringT, typename ...Args>
void TestString(Args&& ...args)
{
	using CharT = StringT::CharType;
	using StringViewT = StringT::StringViewType;

	{
		StringT str = StringT(args...);
		ASSERT(str.IsEmpty());
		ASSERT(str.Size() == 0);
		ASSERT(str.Capacity() > 0);
		ASSERT(str.Data() != nullptr);
	}

	VecTestReserve<StringT>(args...);
	InplaceVecTestResize<StringT>(args...);
	InplaceVecTestFree<StringT>(args...);

	TestVectorBase<StringT>(args...);

	{ // StringTypeBase functions
		StringT str = StringT(args...);
		for (CharT i = 0; i < 16; i++)
			str.EmplaceBack(i);

		TestStringTypeBase<StringT>(str, args...);
	}

	if constexpr (std::is_same_v<CharT, char>)
	{
		{ // From string view
			StringViewT view("Hello World");
			StringT str(view, args...);
			ASSERT(str == "Hello World");
			ASSERT(str == view);
		}

		{ // +=
			StringT str("Hello", args...);
			str += " World!";
			ASSERT(str == "Hello World!");

			str += "";
			ASSERT(str == "Hello World!");

			const StringT str2(" Test", args...);
			str += str2;
			ASSERT(str == "Hello World! Test");

			StringT str3(args...);
			str3 += str;
			ASSERT(str3 == "Hello World! Test");

			StringT str4(args...);
			const StringT str5(args...);
			str4 += str5;
			ASSERT(str4.IsEmpty());
		}

		{ // +
			StringT str("Hello", args...);
			ASSERT(str + " World!" == "Hello World!");

			ASSERT(str + " World!" + "" == "Hello World!");

			const StringT str2(" Test", args...);
			ASSERT(str + " World!" + str2 == "Hello World! Test");

			StringT str3(args...);
			ASSERT(str3 + str == "Hello");

			StringT str4(args...);
			const StringT str5(args...);
			ASSERT((str4 + str5).IsEmpty());
		}

		{ // Move
			StringT str("Here is a very very very very long string", args...);
			StringT str2 = std::move(str);
			ASSERT(str.Size() == 0);
			ASSERT(str == "");

			StringT str3(std::move(str2));
			ASSERT(str2.Size() == 0);
			ASSERT(str2 == "");

			ASSERT(str3 == "Here is a very very very very long string");
		}
	}
	else
	{
		{ // From string view
			StringViewT view(L"Hello World");
			StringT str(view, args...);
			ASSERT(str == L"Hello World");
			ASSERT(str == view);
		}

		{ // +=
			StringT str(L"Hello", args...);
			str += L" World!";
			ASSERT(str == L"Hello World!");

			str += L"";
			ASSERT(str == L"Hello World!");

			const StringT str2(L" Test", args...);
			str += str2;
			ASSERT(str == L"Hello World! Test");

			StringT str3(args...);
			str3 += str;
			ASSERT(str3 == L"Hello World! Test");

			StringT str4(args...);
			const StringT str5(args...);
			str4 += str5;
			ASSERT(str4.IsEmpty());
		}

		{ // +
			StringT str(L"Hello", args...);
			ASSERT(str + L" World!" == L"Hello World!");

			ASSERT(str + L" World!" + L"" == L"Hello World!");

			const StringT str2(L" Test", args...);
			ASSERT(str + L" World!" + str2 == L"Hello World! Test");

			StringT str3(args...);
			ASSERT(str3 + str == L"Hello");

			StringT str4(args...);
			const StringT str5(args...);
			ASSERT((str4 + str5).IsEmpty());
		}

		{ // Move
			StringT str(L"Here is a very very very very long string", args...);
			StringT str2 = std::move(str);
			ASSERT(str.Size() == 0);
			ASSERT(str == L"");

			StringT str3(std::move(str2));
			ASSERT(str2.Size() == 0);
			ASSERT(str2 == L"");

			ASSERT(str3 == L"Here is a very very very very long string");
		}
	}
}

TEST_CASE("Containers/String")
{
	ArenaAllocator arena;
	TestString<String<>>();
	TestString<String<ArenaAllocator>>(arena);
}

TEST_CASE("Containers/WString")
{
	ArenaAllocator arena;
	TestString<WString<>>();
	TestString<WString<ArenaAllocator>>(arena);
}

TEST_CASE("Containers/InplaceString")
{
	ArenaAllocator arena;
	TestString<InplaceString<32>>();
	TestString<InplaceString<32, ArenaAllocator>>(arena);
}

TEST_CASE("Containers/InplaceWString")
{
	ArenaAllocator arena;
	TestString<InplaceWString<32>>();
	TestString<InplaceWString<32, ArenaAllocator>>(arena);
}

TEST_CASE("Containers/HashMap")
{
	HashMap<U32, U32> map = HashMap<U32, U32>();
	map.Insert(1, 2);
	map.Insert(2, 4);
	map.Reserve(300);

	ASSERT(map.Size() == 2);
	ASSERT(!map.IsEmpty());

	ASSERT(map.Find(1) == map.Begin());
	ASSERT(map.Find(2) != map.Begin());

	HashMap<U32, U32> map2 = map.Clone();
	ASSERT(map == map2);

	ASSERT(map2[1] == 2);
	ASSERT(map2[2] == 4);

	map.InsertOrAssign(3, 6);
	map.InsertOrAssign(2, 5);

	ASSERT(map.At(3) == 6);
	ASSERT(map.At(2) == 5);

	map.Erase(1);

	ASSERT(!map.Contains(1));
	ASSERT(map2.Contains(1));

	ASSERT(map.Contains(2));
	ASSERT(map.Contains(3));
	ASSERT(!map.Contains(100));

	for (auto&[k, v] : map)
	{
		ASSERT(k < v);
	}

	map.Clear();
	ASSERT(map.Size() == 0);
	ASSERT(map.IsEmpty());

	ASSERT(typeid(map.GetAllocator()) == typeid(DefaultAllocator));

	{
		StringHashMap<U32> strMap;
		strMap.Insert("Hello", 1);
		strMap.Insert(StringView("Hello2"), 2);
		strMap.Insert(String("Hello3"), 3);

		ASSERT(strMap["Hello"] == 1);
		ASSERT(strMap[StringView("Hello")] == 1);
		ASSERT(strMap[String("Hello")] == 1);

		ASSERT(strMap.At("Hello") == 1);
		ASSERT(strMap.At(StringView("Hello")) == 1);
		ASSERT(strMap.At(String("Hello")) == 1);

		ASSERT(strMap.Find("Hello")->second == 1);
		ASSERT(strMap.Find(StringView("Hello"))->second == 1);
		ASSERT(strMap.Find(String("Hello"))->second == 1);

		ASSERT(strMap.Contains("Hello"));
		ASSERT(strMap.Contains(StringView("Hello")));
		ASSERT(strMap.Contains(String("Hello")));
	}

	{
		StringHashMap<U32> strMap_;
		strMap_.Insert("Hello", 1);
		const StringHashMap<U32>& strMap = strMap_;

		ASSERT(strMap.At("Hello") == 1);
		ASSERT(strMap.At(StringView("Hello")) == 1);
		ASSERT(strMap.At(String("Hello")) == 1);

		ASSERT(strMap.Find("Hello")->second == 1);
		ASSERT(strMap.Find(StringView("Hello"))->second == 1);
		ASSERT(strMap.Find(String("Hello"))->second == 1);

		ASSERT(strMap.Contains("Hello"));
		ASSERT(strMap.Contains(StringView("Hello")));
		ASSERT(strMap.Contains(String("Hello")));
	}
}

TEST_CASE("Containers/HashSet")
{
	HashSet<U32> set = HashSet<U32>();
	set.Insert(1);
	set.Insert(2);
	set.Reserve(300);

	ASSERT(set.Size() == 2);
	ASSERT(!set.IsEmpty());

	HashSet<U32> set2 = set.Clone();
	ASSERT(set == set2);

	set.Erase(1);

	ASSERT(!set.Contains(1));
	ASSERT(set2.Contains(1));

	ASSERT(set.Contains(2));
	ASSERT(!set.Contains(100));

	for (auto& k : set)
	{
		ASSERT(k > 0);
	}

	set.Clear();
	ASSERT(set.Size() == 0);
	ASSERT(set.IsEmpty());

	ASSERT(typeid(set.GetAllocator()) == typeid(DefaultAllocator));
}

TEST_CASE("Containers/UniquePtr")
{
	{
		UniquePtr<String<>> ptr;
		ASSERT(!ptr);
		ASSERT(ptr.IsEmpty());
		ASSERT(ptr.Get() == nullptr);

		auto ptr2 = ptr.Clone();
		ASSERT(!ptr2);
		ASSERT(ptr2.IsEmpty());
		ASSERT(ptr2.Get() == nullptr);

		String<>* newStr = (String<>*)DefaultAllocator{}.Allocate(sizeof(String<>), alignof(String<>));
		ptr2.Assign(newStr);
		ASSERT(ptr2);
		ASSERT(!ptr2.IsEmpty());
		ASSERT(ptr2.Get() == newStr);
	}

	{
		UniquePtr<String<>> ptr = MakeUnique<String<>>("Hello");
		ASSERT(ptr);
		ASSERT(!ptr.IsEmpty());
		ASSERT(ptr->Size() == 5);
		ASSERT(*ptr == "Hello");

		const UniquePtr<String<>> ptr2 = std::move(ptr);
		ASSERT(!ptr);
		ASSERT(ptr.IsEmpty());
		ASSERT(ptr.Get() == nullptr);
		ASSERT(ptr2.Get() != nullptr);
		ASSERT(ptr2);
		ASSERT(!ptr2.IsEmpty());
		ASSERT(ptr2->Size() == 5);
		ASSERT(*ptr2 == "Hello");
	}

	{
		UniquePtr<U32> ptr((U32*)DefaultAllocator{}.Allocate(sizeof(U32), alignof(U32)));
		ASSERT(ptr);
		ASSERT(ptr.Get() != nullptr);

		ptr.Free();
		ASSERT(!ptr);
		ASSERT(ptr.Get() == nullptr);
	}

	{
		ArenaAllocator alloc;
		auto ptr = AllocateUnique<String<>, ArenaAllocator>(alloc, "Hello");
		ASSERT(&ptr.GetAllocator() == &alloc);
		ASSERT(*ptr == "Hello");

		auto ptr2 = ptr.Clone();
		ASSERT(&ptr2.GetAllocator() == &alloc);
		ASSERT(*ptr2 == "Hello");
		ASSERT(ptr.Get() != ptr2.Get());
		ASSERT(*ptr == *ptr);
	}
}

TEST_CASE("Containers/SharedPtr")
{
	{
		WeakPtr<String<>> weakPtr;
		ASSERT(!weakPtr);
		ASSERT(weakPtr.IsEmpty());
	}

	{
		SharedPtr<String<>> ptr;
		ASSERT(!ptr);
		ASSERT(ptr.IsEmpty());
		ASSERT(ptr.Get() == nullptr);
		ASSERT(ptr.NumRefs() == 0);
		ASSERT(ptr.NumWeakRefs() == 0);

		WeakPtr<String<>> weakPtr = ptr.GetWeak();
		ASSERT(!weakPtr);
		ASSERT(weakPtr.IsEmpty());
	}

	WeakPtr<String<>> weakPtr;
	ASSERT(!weakPtr);
	ASSERT(weakPtr.IsEmpty());

	{
		String<>* str = new String("hello");
		SharedPtr<String<>> ptr = MakeSharedFromPtr<String<>, DefaultAllocator>(str);
		ASSERT(ptr);
		ASSERT(!ptr.IsEmpty());
		ASSERT(ptr.Get() == str);
		ASSERT(ptr.NumRefs() == 1);
		ASSERT(ptr.NumWeakRefs() == 0);

		weakPtr = ptr.GetWeak();
		ASSERT(weakPtr);
		ASSERT(!weakPtr.IsEmpty());
		ASSERT(ptr.NumWeakRefs() == 1);
		ASSERT(weakPtr.Lock().Get() == str);

		{
			WeakPtr<String<>> weakPtr2 = weakPtr;
			ASSERT(weakPtr2);
			ASSERT(!weakPtr2.IsEmpty());
			ASSERT(ptr.NumWeakRefs() == 2);
			ASSERT(weakPtr2.Lock().Get() == str);
		}

		SharedPtr<String<>> ptr2 = ptr;
		ASSERT(ptr2);
		ASSERT(!ptr2.IsEmpty());
		ASSERT(ptr2.Get() == str);
		ASSERT(ptr2.NumRefs() == 2);
		ASSERT(ptr2.NumWeakRefs() == 1);

		SharedPtr<String<>> ptr3 = ptr2;
		ASSERT(ptr3);
		ASSERT(!ptr3.IsEmpty());
		ASSERT(ptr3.Get() == str);
		ASSERT(ptr3.NumRefs() == 3);
		ASSERT(ptr3.NumWeakRefs() == 1);
	}

	ASSERT(!weakPtr);
	ASSERT(weakPtr.IsEmpty());

	{
		SharedPtr<String<>> ptr = AllocateShared<String<>, DefaultAllocator>(DefaultAllocator{}, "Hello");
		ASSERT(ptr);
		ASSERT(!ptr.IsEmpty());
		ASSERT(ptr->Size() == 5);
		ASSERT(*ptr == "Hello");
		ASSERT(ptr.NumRefs() == 1);
		ASSERT(ptr.NumWeakRefs() == 0);

		SharedPtr<String<>> ptr2 = ptr;
		ASSERT(ptr2);
		ASSERT(!ptr2.IsEmpty());
		ASSERT(ptr2->Size() == 5);
		ASSERT(*ptr2 == "Hello");
		ASSERT(ptr.NumRefs() == 2);
		ASSERT(ptr.NumWeakRefs() == 0);

		{
			SharedPtr<String<>> ptr3 = ptr2;
			ASSERT(ptr3);
			ASSERT(!ptr3.IsEmpty());
			ASSERT(ptr3->Size() == 5);
			ASSERT(*ptr3 == "Hello");
			ASSERT(ptr.NumRefs() == 3);
			ASSERT(ptr.NumWeakRefs() == 0);
		}

		ASSERT(ptr.NumRefs() == 2);
		ASSERT(ptr.NumWeakRefs() == 0);
	}

	{
		ArenaAllocator arena;
		SharedPtr<String<>> ptr = AllocateShared<String<>, ArenaAllocator>(arena, "Hello");
		ASSERT(ptr);
		ASSERT(!ptr.IsEmpty());
		ASSERT(ptr->Size() == 5);
		ASSERT(*ptr == "Hello");

		SharedPtr<String<>> ptr2 = ptr;
		ASSERT(ptr2);
		ASSERT(!ptr2.IsEmpty());
		ASSERT(ptr2->Size() == 5);
		ASSERT(*ptr2 == "Hello");

		SharedPtr<String<>> ptr3 = ptr2;
		ASSERT(ptr3);
		ASSERT(!ptr3.IsEmpty());
		ASSERT(ptr3->Size() == 5);
		ASSERT(*ptr3 == "Hello");

		SharedPtr<String<>> ptr4 = std::move(ptr3);
		ASSERT(ptr4);
		ASSERT(!ptr4.IsEmpty());
		ASSERT(ptr4->Size() == 5);
		ASSERT(*ptr4 == "Hello");
	}
}

TEST_CASE("Containers/AtomicSharedPtr")
{
	{
		AtomicWeakPtr<String<>> weakPtr;
		ASSERT(!weakPtr);
		ASSERT(weakPtr.IsEmpty());
	}

	{
		AtomicSharedPtr<String<>> ptr;
		ASSERT(!ptr);
		ASSERT(ptr.IsEmpty());
		ASSERT(ptr.Get() == nullptr);
		ASSERT(ptr.NumRefs() == 0);
		ASSERT(ptr.NumWeakRefs() == 0);

		AtomicWeakPtr<String<>> weakPtr = ptr.GetWeak();
		ASSERT(!weakPtr);
		ASSERT(weakPtr.IsEmpty());
	}

	AtomicWeakPtr<String<>> weakPtr;
	ASSERT(!weakPtr);
	ASSERT(weakPtr.IsEmpty());

	{
		String<>* str = new String("hello");
		AtomicSharedPtr<String<>> ptr = MakeAtomicSharedFromPtr<String<>, DefaultAllocator>(str);
		ASSERT(ptr);
		ASSERT(!ptr.IsEmpty());
		ASSERT(ptr.Get() == str);
		ASSERT(ptr.NumRefs() == 1);
		ASSERT(ptr.NumWeakRefs() == 0);

		weakPtr = ptr.GetWeak();
		ASSERT(weakPtr);
		ASSERT(!weakPtr.IsEmpty());
		ASSERT(ptr.NumWeakRefs() == 1);
		ASSERT(weakPtr.Lock().Get() == str);

		{
			AtomicWeakPtr<String<>> weakPtr2 = weakPtr;
			ASSERT(weakPtr2);
			ASSERT(!weakPtr2.IsEmpty());
			ASSERT(ptr.NumWeakRefs() == 2);
			ASSERT(weakPtr2.Lock().Get() == str);
		}

		AtomicSharedPtr<String<>> ptr2 = ptr;
		ASSERT(ptr2);
		ASSERT(!ptr2.IsEmpty());
		ASSERT(ptr2.Get() == str);
		ASSERT(ptr2.NumRefs() == 2);
		ASSERT(ptr2.NumWeakRefs() == 1);

		AtomicSharedPtr<String<>> ptr3 = ptr2;
		ASSERT(ptr3);
		ASSERT(!ptr3.IsEmpty());
		ASSERT(ptr3.Get() == str);
		ASSERT(ptr3.NumRefs() == 3);
		ASSERT(ptr3.NumWeakRefs() == 1);
	}

	ASSERT(!weakPtr);
	ASSERT(weakPtr.IsEmpty());

	{
		AtomicSharedPtr<String<>> ptr = AllocateAtomicShared<String<>, DefaultAllocator>(DefaultAllocator{}, "Hello");
		ASSERT(ptr);
		ASSERT(!ptr.IsEmpty());
		ASSERT(ptr->Size() == 5);
		ASSERT(*ptr == "Hello");
		ASSERT(ptr.NumRefs() == 1);
		ASSERT(ptr.NumWeakRefs() == 0);

		AtomicSharedPtr<String<>> ptr2 = ptr;
		ASSERT(ptr2);
		ASSERT(!ptr2.IsEmpty());
		ASSERT(ptr2->Size() == 5);
		ASSERT(*ptr2 == "Hello");
		ASSERT(ptr.NumRefs() == 2);
		ASSERT(ptr.NumWeakRefs() == 0);

		{
			AtomicSharedPtr<String<>> ptr3 = ptr2;
			ASSERT(ptr3);
			ASSERT(!ptr3.IsEmpty());
			ASSERT(ptr3->Size() == 5);
			ASSERT(*ptr3 == "Hello");
			ASSERT(ptr.NumRefs() == 3);
			ASSERT(ptr.NumWeakRefs() == 0);
		}

		ASSERT(ptr.NumRefs() == 2);
		ASSERT(ptr.NumWeakRefs() == 0);
	}

	{
		ArenaAllocator arena;
		AtomicSharedPtr<String<>> ptr = AllocateAtomicShared<String<>, ArenaAllocator>(arena, "Hello");
		ASSERT(ptr);
		ASSERT(!ptr.IsEmpty());
		ASSERT(ptr->Size() == 5);
		ASSERT(*ptr == "Hello");

		AtomicSharedPtr<String<>> ptr2 = ptr;
		ASSERT(ptr2);
		ASSERT(!ptr2.IsEmpty());
		ASSERT(ptr2->Size() == 5);
		ASSERT(*ptr2 == "Hello");

		AtomicSharedPtr<String<>> ptr3 = ptr2;
		ASSERT(ptr3);
		ASSERT(!ptr3.IsEmpty());
		ASSERT(ptr3->Size() == 5);
		ASSERT(*ptr3 == "Hello");

		AtomicSharedPtr<String<>> ptr4 = std::move(ptr3);
		ASSERT(ptr4);
		ASSERT(!ptr4.IsEmpty());
		ASSERT(ptr4->Size() == 5);
		ASSERT(*ptr4 == "Hello");
	}


	{ // Test for race conditions
		AtomicSharedPtr<String<>> ptr1 = AllocateAtomicShared<String<>, DefaultAllocator>(DefaultAllocator{}, "Hello1");
		AtomicSharedPtr<String<>> ptr2 = AllocateAtomicShared<String<>, DefaultAllocator>(DefaultAllocator{}, "Hello2");
		AtomicSharedPtr<String<>> ptr3 = AllocateAtomicShared<String<>, DefaultAllocator>(DefaultAllocator{}, "Hello3");
		AtomicSharedPtr<String<>> ptr4 = AllocateAtomicShared<String<>, DefaultAllocator>(DefaultAllocator{}, "Hello4");
		AtomicSharedPtr<String<>> ptr5 = AllocateAtomicShared<String<>, DefaultAllocator>(DefaultAllocator{}, "Hello5");

		Vector<std::thread> threads;
		threads.Reserve(100);

		for (int ti = 0; ti < 100; ti++)
		{
			threads.EmplaceBack([&] {
				for (int i = 0; i < 100'000; i++)
				{
					AtomicSharedPtr<String<>> ptr = ptr1;
					ASSERT(ptr);
					ASSERT(*ptr == "Hello1");
					ASSERT(ptr.GetWeak().Lock().Get() == ptr.Get());

					ptr = ptr2;
					ASSERT(ptr);
					ASSERT(*ptr == "Hello2");
					ASSERT(ptr.GetWeak().Lock().Get() == ptr.Get());

					ptr = ptr3;
					ASSERT(ptr);
					ASSERT(*ptr == "Hello3");
					ASSERT(ptr.GetWeak().Lock().Get() == ptr.Get());

					ptr = ptr4;
					ASSERT(ptr);
					ASSERT(*ptr == "Hello4");
					ASSERT(ptr.GetWeak().Lock().Get() == ptr.Get());

					ptr = ptr5;
					ASSERT(ptr);
					ASSERT(*ptr == "Hello5");
					ASSERT(ptr.GetWeak().Lock().Get() == ptr.Get());
				}
			});
		}

		for (auto& t : threads)
		{
			t.join();
		}
	}
}

TEST_CASE("Containers/Function")
{
	{ // Empty
		Function<void()> func;
		ASSERT(!func);
	}

	{ // Func ptr
		Function<int()> func = +[]() { return 0; };
		ASSERT(func);
		ASSERT(func() == 0);

		func = +[]() { return 1; };
		ASSERT(func);
		ASSERT(func() == 1);
	}

	{ // Lambda with capture
		int value = 2;
		Function<int()> func = [&] {return value; };
		ASSERT(func);
		ASSERT(func() == 2);
		
		value = 3;
		func = [&] { return value; };
		ASSERT(func);
		ASSERT(func() == 3);
	}

	{ // Lambda too big for inline
		struct big {
			U64 a = 1;
			U64 b = 2;
			U64 c = 3;
			U64 d = 4;
		} bigData;

		Function<int()> func = Function<int()>::Allocate<DefaultAllocator>([bigData] { return (int)bigData.a; });
		ASSERT(func);
		ASSERT(func() == 1);

		func = Function<int()>::Allocate<DefaultAllocator>([bigData] { return (int)bigData.b; });
		ASSERT(func);
		ASSERT(func() == 2);
	}
}

template<typename DequeT>
void TestDeque(AllocatorRef<typename DequeT::AllocatorType> allocator)
{
	using ValueType = typename DequeT::ValueType;
	using IndexType = typename DequeT::IndexType;

	DequeT deque = DequeT(allocator);

	// Test IsEmpty, Size, Capacity
	ASSERT(deque.IsEmpty());
	ASSERT(deque.Size() == 0);
	ASSERT(deque.Capacity() == 0);

	// Test PushBack and EmplaceBack
	for (IndexType i = 0; i < 2048; ++i)
		deque.PushBack(static_cast<ValueType>(i));
	ASSERT(!deque.IsEmpty());
	ASSERT(deque.Size() == 2048);
	for (IndexType i = 0; i < 2048; ++i)
		ASSERT(deque[i] == static_cast<ValueType>(i));

	deque.EmplaceBack(static_cast<ValueType>(2048));
	ASSERT(deque.Size() == 2049);
	ASSERT(deque[2048] == static_cast<ValueType>(2048));

	// Test PushFront and EmplaceFront
	deque.PushFront(static_cast<ValueType>(-1));
	ASSERT(deque.Size() == 2050);
	ASSERT(deque[0] == static_cast<ValueType>(-1));

	for (IndexType i = 0; i < 1000; i++)
		deque.EmplaceFront(-static_cast<ValueType>(i));

	ASSERT(deque.Size() == 3050);
	for (IndexType i = 0; i < 1000; i++) {
		ASSERT(deque[i] == -999 + static_cast<ValueType>(i));
	}

	// Test PopBack and PopFront
	for (IndexType i = 0; i < 1000; i++) {
		ValueType back = deque.PopBack();
		ASSERT(back == static_cast<ValueType>(2048 - i));
	}
	ASSERT(deque.Size() == 2050);

	for (IndexType i = 0; i < 1000; i++) {
		ValueType front = deque.PopFront();
		ASSERT(front == -999 + static_cast<ValueType>(i));
	}
	ASSERT(deque.Size() == 1050);

	// Test First and Last
	ASSERT(deque.First() == static_cast<ValueType>(-1));
	ASSERT(deque.Last() == static_cast<ValueType>(1048));

	// Test Contains
	ASSERT(deque.Contains(static_cast<ValueType>(10)));
	ASSERT(!deque.Contains(static_cast<ValueType>(10000)));

	// Test TryFind
	ValueType* found = deque.TryFind(static_cast<ValueType>(10));
	ASSERT(found && *found == static_cast<ValueType>(10));

	ValueType* notFound = deque.TryFind(static_cast<ValueType>(10000));
	ASSERT(notFound == nullptr);

	// Test Clear
	deque.Clear();
	ASSERT(deque.IsEmpty());
	ASSERT(deque.Size() == 0);

	// Test Reserve and Resize
	deque.Reserve(32);
	ASSERT(deque.Capacity() >= 32);

	deque.Resize(1024, static_cast<ValueType>(42));
	ASSERT(deque.Size() == 1024);
	for (IndexType i = 0; i < 1024; ++i)
		ASSERT(deque[i] == static_cast<ValueType>(42));

	deque.Resize(4);
	ASSERT(deque.Size() == 4);

	// Test Clone
	DequeT clone = deque.Clone();
	ASSERT(clone.Size() == deque.Size());
	for (IndexType i = 0; i < clone.Size(); ++i)
		ASSERT(clone[i] == deque[i]);

	// Test Begin and End
	IndexType index = 0;
	for (auto it = deque.Begin(); it != deque.End(); ++it)
		ASSERT(*it == deque[index++]);

	// Test Iterators with ConstDeque
	const DequeT& constDeque = deque;
	index = 0;
	for (auto it = constDeque.CBegin(); it != constDeque.CEnd(); ++it)
		ASSERT(*it == deque[index++]);

	// Test ShrinkToFit and Free
	deque.ShrinkToFit();
	ASSERT(deque.Capacity() >= deque.Size());

	deque.Free();
	ASSERT(deque.IsEmpty());
	ASSERT(deque.Size() == 0);
	ASSERT(deque.Capacity() == 0);
}

TEST_CASE("Containers/Deque")
{
	ArenaAllocator arena;

	// Test SmallDeque with default and arena allocators
	TestDeque<SmallDeque<S64>>(DefaultAllocator{});
	TestDeque<SmallDeque<S64, ArenaAllocator>>(arena);

	// Test Deque with default and arena allocators
	TestDeque<Deque<S64>>(DefaultAllocator{});
	TestDeque<Deque<S64, ArenaAllocator>>(arena);

	// Test BigDeque with default and arena allocators
	TestDeque<BigDeque<S64>>(DefaultAllocator{});
	TestDeque<BigDeque<S64, ArenaAllocator>>(arena);
}

template<typename StackT>
void TestStack(AllocatorRef<typename StackT::AllocatorType> allocator)
{
	using ValueType = typename StackT::ValueType;
	using IndexType = typename StackT::IndexType;

	StackT stack = StackT(allocator);

	ASSERT(stack.IsEmpty());
	ASSERT(stack.Size() == 0);

	for (ValueType i = 0; i < 1024; ++i) {
		stack.PushBack(i);
		ASSERT(stack.Peek() == i);
	}

	for (ValueType i = 1024; i < 2048; ++i) {
		stack.EmplaceBack(i);
		ASSERT(stack.Peek() == i);
	}

	ASSERT(!stack.IsEmpty());
	ASSERT(stack.Size() == 2048);

	const StackT& constStack = stack;
	ASSERT(constStack.Peek() == 2047);

	StackT copy = stack.Clone();
	ASSERT(copy.Size() == stack.Size());
	for (ValueType i = 0; i < 2048; ++i) {
		ASSERT(copy.PopBack() == 2047 - i);
	}

	for (ValueType i = 0; i < 1024; ++i) {
		ASSERT(stack.PopBack() == 2047 - i);
	}

	stack.ShrinkToFit();
	ASSERT(stack.Size() == 1024);

	for (ValueType i = 0; i < 1024; ++i) {
		ASSERT(stack.PopBack() == 1023 - i);
	}

	ASSERT(stack.IsEmpty());
	ASSERT(stack.Size() == 0);

	for (ValueType i = 0; i < 1024; ++i) {
		stack.PushBack(i);
		ASSERT(stack.Peek() == i);
	}

	stack.Clear();

	ASSERT(stack.IsEmpty());
	ASSERT(stack.Size() == 0);
}

TEST_CASE("Containers/Stack")
{
	ArenaAllocator arena;

	TestStack<SmallStack<S64>>(DefaultAllocator{});
	TestStack<SmallStack<S64, ArenaAllocator>>(arena);

	TestStack<Stack<S64>>(DefaultAllocator{});
	TestStack<Stack<S64, ArenaAllocator>>(arena);

	TestStack<BigStack<S64>>(DefaultAllocator{});
	TestStack<BigStack<S64, ArenaAllocator>>(arena);
}

TEST_CASE("Containers/RingBuffer")
{
	RingBuffer buffer(256);
	void* ptr1 = buffer.Allocate(200, 8);
	memset(ptr1, 42, 200);

	void* ptr2 = buffer.Allocate(12, 8);
	memset(ptr1, 5, 12);

	void* ptr3 = buffer.Allocate(200, 8);
	void* ptr4 = buffer.Allocate(12, 8);

	ASSERT(ptr1 == ptr3);
	ASSERT(ptr2 == ptr4);
}

TEST_CASE("Containers/NoDestructor")
{
	struct DontDestroy
	{
		int value = 3;
		~DontDestroy() { assert(false); }
	};

	NoDestructor<DontDestroy> dontDestroy;

	ASSERT(dontDestroy->value == 3);
	ASSERT((*dontDestroy).value == 3);
}

TEST_CASE("Containers/ConstInit")
{
	constinit static Deque<U32> deque;
	//constinit static Function<void()> function; // https://en.cppreference.com/w/cpp/memory/construct_at
	//constinit static HashMap<U32, U32> hashMap;
	//constinit static HashSet<U32> hashSet;
	constinit static UniquePtr<U32> uniquePtr;
	constinit static SharedPtr<U32> sharedPtr;
	constinit static AtomicSharedPtr<U32> atomicSharedPtr;
	constinit static Span<U32> span;
	constinit static Stack<U32> stack;
	constinit static String<> string;
	constinit static WString<> wstring;
	constinit static InplaceString<32> inplaceString;
	constinit static InplaceWString<32> inplaceWString;
	constinit static Vector<U32> vector;
	constinit static InplaceVector<U32, 32> inplaceVector;
	constinit static FixedVector<U32, 32> fixedVector;
}
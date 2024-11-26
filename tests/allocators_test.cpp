#include <tests/test_utils.hpp>

#include <rexcore/allocators.hpp>

using namespace RexCore;

template<typename T>
void TestAllocator()
{
	ASSERT(IAllocator<T>);
	T allocator{};

	void* ptr = allocator.Allocate(32, 4);
	ASSERT(ptr != nullptr);
	ASSERT((size_t)ptr % 4 == 0);

	MemCopy((void*)"_TestAllocator_", ptr, 16);

	ptr = allocator.Reallocate(ptr, 32, 64, 8);
	ASSERT(ptr != nullptr);
	ASSERT((size_t)ptr % 8 == 0);

	ASSERT(strcmp("_TestAllocator_", (const char*)ptr) == 0);

	allocator.Free(ptr, 64);
}

TEST_CASE("Allocators/PageFunctions")
{
	ASSERT(PageSize > 0);
	
	void* ptr = ReservePages(3);
	ASSERT(ptr != nullptr);
	CommitPages(ptr, 3);
	MemCopy((void*)"PageFunctions", ptr, 13);
	
	DecommitPages(ptr, 3);
	ReleasePages(ptr, 3);
}

TEST_CASE("Allocators") 
{
	TestAllocator<MallocAllocator>();
	TestAllocator<PageAllocator>();
	TestAllocator<ArenaAllocator>();

	{ // Arena
		ArenaAllocator arena;
		void* ptr1 = arena.Allocate(32, 4);
		void* ptr2 = arena.Allocate(100024, 24);
		void* ptr3 = arena.Allocate(64, 7);

		ASSERT(ptr1 != nullptr);
		ASSERT(ptr1 < ptr2);
		ASSERT(ptr2 < ptr3);
		
		arena.Reset();

		void* ptr4 = arena.Allocate(32, 24);
		void* ptr5 = arena.Allocate(100024, 4);
		void* ptr6 = arena.Allocate(64, 7);
		ASSERT(ptr4 == ptr1);
		ASSERT(ptr5 == ptr2);
		ASSERT(ptr6 == ptr3);
	}
}

TEST_CASE("Allocators/STD_Adapter")
{
	{ // stateful
		ArenaAllocator arena;
		std::vector<U32, StdAllocatorAdaptor<U32, ArenaAllocator>> vec(arena);

		for (U32 i = 0; i < 100; ++i)
			vec.push_back(i);

		for (U32 i = 0; i < 100; ++i)
			ASSERT(vec[i] == i);
	}

	{ // stateless
		std::vector<U32, StdAllocatorAdaptor<U32, MallocAllocator>> vec;

		for (U32 i = 0; i < 100; ++i)
			vec.push_back(i);

		for (U32 i = 0; i < 100; ++i)
			ASSERT(vec[i] == i);
	}
}

TEST_CASE("Allocators/PoolAlocator")
{
	struct vec2 {
		U32 x, y;
	};

	{
		PoolAllocator<vec2> pool;

		vec2* ptr1 = pool.AllocateItem();
		ASSERT(ptr1 != nullptr);
		ptr1->x = 1;
		ptr1->y = 2;

		vec2* ptr2 = pool.AllocateItem();
		ASSERT(ptr2 != nullptr);
		ptr2->x = 3;
		ptr2->y = 4;

		vec2* ptr3 = pool.AllocateItem();
		ASSERT(ptr3 != nullptr);
		ptr3->x = 5;
		ptr3->y = 6;

		ASSERT(ptr1 != ptr2);
		ASSERT(ptr2 != ptr3);
		
		pool.FreeItem(ptr2);
		ASSERT(ptr1->x == 1);
		ASSERT(ptr1->y == 2);
		ASSERT(ptr3->x == 5);
		ASSERT(ptr3->y == 6);

		vec2* ptr4 = pool.AllocateItem();
		ASSERT(ptr4 == ptr2);

		vec2* ptr5 = pool.AllocateItem();
		ASSERT(ptr5 != nullptr);

		pool.FreeItem(ptr1);
		pool.FreeItem(ptr3);
		pool.FreeItem(ptr4);
		pool.FreeItem(ptr5);
	}
}
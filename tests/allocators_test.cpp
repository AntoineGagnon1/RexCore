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
}
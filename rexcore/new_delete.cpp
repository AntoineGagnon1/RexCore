#include <rexcore/allocators.hpp>

#include <stdlib.h>

void* operator new(size_t size)
{
	void* ptr = RexCore::DefaultAllocator{}.Allocate(size, 8);
	REX_CORE_ASSERT(ptr != nullptr);
	return ptr;
}

void operator delete(void* ptr, size_t size)
{
	RexCore::DefaultAllocator{}.Free(ptr, static_cast<RexCore::U64>(size));
}
#include <rexcore/allocators.hpp>

#include <stdlib.h>

void* operator new(size_t size)
{
	void* ptr = malloc(size);
	REX_CORE_ASSERT(ptr != nullptr);
	RexCore::TrackAlloc(ptr, static_cast<RexCore::U64>(size), RexCore::AllocSourceLocation::current());
	return ptr;
}

void operator delete(void* ptr, size_t size)
{
	RexCore::TrackFree(ptr, static_cast<RexCore::U64>(size), RexCore::AllocSourceLocation::current());
	free(ptr);
}
#pragma once

#ifndef REX_CORE_ASSERT
#include <cassert>
#define REX_CORE_ASSERT(...) assert(__VA_ARGS__)
#endif // !REX_CORE_ASSERT

// Called when a Free() is called on an address that was not Alloc()ed
#ifndef REX_CORE_FREE_NO_ALLOC
#define REX_CORE_FREE_NO_ALLOC(ptr, size, loc) REX_CORE_ASSERT(false)
#endif // !REX_CORE_LOG

// Called when a Free() is called with a different size than the allocated size
#ifndef REX_CORE_ASYMMETRIC_FREE
#define REX_CORE_ASYMMETRIC_FREE(ptr, freeSize, freeLocation, allocSize, allocLocation) REX_CORE_ASSERT(false)
#endif // !REX_CORE_LOG

// Called when an Alloc() returns an already allocated address or a page is commited more than once
#ifndef REX_CORE_ALLOC_NO_FREE
#define REX_CORE_ALLOC_NO_FREE(ptr, oldSize, oldLocation, newSize, newLocation) REX_CORE_ASSERT(false)
#endif // !REX_CORE_LOG

// Called when a non-reserved page is committed
#ifndef REX_CORE_COMMIT_NO_RESERVE
#define REX_CORE_COMMIT_NO_RESERVE(ptr, loc) REX_CORE_ASSERT(false)
#endif // !REX_CORE_LOG

// Called when a leak is detected when CheckForLeaks() is called
#ifndef REX_CORE_LEAK
#define REX_CORE_LEAK(ptr, size, allocLocation) REX_CORE_ASSERT(false)
#endif // !REX_CORE_LOG
#pragma once

// #define REX_CORE_TRACK_ALLOCS
// #define REX_CORE_TRACE_ENABLED

// Only available if std::stacktrace is available
// WARNING : makes allocations very slow, but usefull to find leaks
// #define REX_CORE_TRACK_ALLOCS_TRACE

#ifdef REX_CORE_CONFIG_INCLUDE
#include REX_CORE_CONFIG_INCLUDE
#endif // REX_CORE_CONFIG_INCLUDE

#ifndef REX_CORE_ASSERT
#include <cassert>
#define REX_CORE_ASSERT(cond, ...) assert(cond)
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

// Called when a leak is detected when CheckForLeaks() is called
#ifndef REX_CORE_LEAK
#define REX_CORE_LEAK(ptr, size, allocLocation) REX_CORE_ASSERT(false)
#endif // !REX_CORE_LOG
#include <rexcore/allocators.hpp>
#include <rexcore/system_headers.hpp>

#include <rexcore/containers/map.hpp>
#include <rexcore/containers/smart_ptrs.hpp>

#ifdef REX_CORE_TRACK_ALLOCS_TRACE
	#if __cpp_lib_stacktrace == 202011L && __cpp_lib_formatters	== 202302L
		#include <stacktrace>
	#else
		#error "REX_CORE_TRACK_ALLOCS_TRACE requires std::stacktrace"
	#endif
#endif

namespace RexCore
{
#ifdef REX_CORE_WIN32
	static U64 GetPageSize()
	{
		SYSTEM_INFO info;
		GetSystemInfo(&info);
		return static_cast<U64>(info.dwPageSize);
	}

	void* ReservePages(U64 numPages)
	{
		REX_CORE_TRACE_FUNC();
#if (_MSC_VER <= 1900) || WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
		void* ptr = VirtualAlloc(nullptr, numPages * PageSize, MEM_RESERVE, PAGE_READWRITE);
#else
		void* ptr = VirtualAllocFromApp(nullptr, numPages * PageSize, MEM_RESERVE, PAGE_READWRITE);
#endif
		REX_CORE_ASSERT(ptr != nullptr);
		return ptr;
	}

	void ReleasePages(void* address, [[maybe_unused]]U64 numPages)
	{
		REX_CORE_TRACE_FUNC();
		REX_CORE_ASSERT(VirtualFree(address, 0u, MEM_RELEASE));
	}

	void CommitPagesUntracked(void* address, U64 numPages)
	{
		REX_CORE_TRACE_FUNC();
#if (_MSC_VER <= 1900) || WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
		void* newPtr = VirtualAlloc(address, numPages * PageSize, MEM_COMMIT, PAGE_READWRITE);
#else
		void* newPtr = VirtualAllocFromApp(address, numPages * PageSize, MEM_COMMIT, PAGE_READWRITE);
#endif
		REX_CORE_ASSERT(newPtr != nullptr && address == newPtr);
	}

	void DecommitPagesUntracked(void* address, U64 numPages)
	{
		REX_CORE_TRACE_FUNC();
		REX_CORE_ASSERT(VirtualFree(address, numPages * PageSize, MEM_DECOMMIT));
	}


#else
#error "Page allocation functions not implemented for this platform"
#endif

	const U64 PageSize = GetPageSize();

#ifdef REX_CORE_TRACK_ALLOCS
	using AllocTrackAllocator = NonTracking<MallocAllocator>;

#ifdef REX_CORE_TRACK_ALLOCS_TRACE
	using StackTraceType = std::basic_stacktrace<StdAllocatorAdaptor<std::stacktrace_entry, AllocTrackAllocator>>;
#endif
	struct Alloc
	{
		U64 size;
#ifdef REX_CORE_TRACK_ALLOCS_TRACE
		StackTraceType loc;
#else
		AllocSourceLocation loc;
#endif
	};

	// TODO : should use a faster free-list allocator
	static UniquePtr<HashMap<void*, Alloc, AllocTrackAllocator>, AllocTrackAllocator> s_aliveAlloc;

	void TrackAlloc(void* ptr, U64 size, [[maybe_unused]] AllocSourceLocation loc)
	{
		REX_CORE_TRACE_FUNC();
		if (!s_aliveAlloc)
			return;

#ifdef REX_CORE_TRACK_ALLOCS_TRACE
		const bool inserted = s_aliveAlloc->Insert(ptr, Alloc{ size, StackTraceType::current()}).second;
#else
		const bool inserted = s_aliveAlloc->Insert(ptr, Alloc{ size, loc }).second;
#endif

		if (!inserted)
		{
			auto found = s_aliveAlloc->Find(ptr);
			REX_CORE_ALLOC_NO_FREE(ptr, found->second.size, found->second.loc, size, loc);
		}
	}

	void TrackFree(void* ptr, U64 size, [[maybe_unused]]AllocSourceLocation loc)
	{
		REX_CORE_TRACE_FUNC();
		if (!s_aliveAlloc)
			return;

		auto found = s_aliveAlloc->Find(ptr);
		if (found == s_aliveAlloc->end())
		{
			REX_CORE_FREE_NO_ALLOC(ptr, size, loc);
			return;
		}
		else
		{
			if (size != 0 && found->second.size != size)
			{
				REX_CORE_ASYMMETRIC_FREE(ptr, size, loc, found->second.size, found->second.loc);
			}
			s_aliveAlloc->Erase(ptr);
		}
	}

	void StartTrackingMemory()
	{
		s_aliveAlloc = AllocateUnique<decltype(s_aliveAlloc)::ValueType, AllocTrackAllocator>(AllocTrackAllocator{});
	}

	bool CheckForLeaks()
	{
		REX_CORE_ASSERT(s_aliveAlloc, "RexCore::StartTrackingMemory() was not called");

		for (auto&[ptr, alloc] : *s_aliveAlloc)
		{
			REX_CORE_LEAK(ptr, alloc.size, alloc.loc);
		}

		const bool leaks = !s_aliveAlloc->IsEmpty();
		s_aliveAlloc.Free();

		return leaks;
	}
#endif
}
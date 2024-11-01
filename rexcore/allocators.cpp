#include <rexcore/allocators.hpp>
#include <rexcore/system_headers.hpp>

#include <unordered_map>
#include <memory>

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
		REX_CORE_ASSERT(VirtualFree(address, 0u, MEM_RELEASE));
	}

	void CommitPagesUntracked(void* address, U64 numPages)
	{
#if (_MSC_VER <= 1900) || WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
		void* newPtr = VirtualAlloc(address, numPages * PageSize, MEM_COMMIT, PAGE_READWRITE);
#else
		void* newPtr = VirtualAllocFromApp(address, numPages * PageSize, MEM_COMMIT, PAGE_READWRITE);
#endif
		REX_CORE_ASSERT(newPtr != nullptr && address == newPtr);
	}

	void DecommitPagesUntracked(void* address, U64 numPages)
	{
		REX_CORE_ASSERT(VirtualFree(address, numPages * PageSize, MEM_DECOMMIT));
	}


#else
#error "Page allocation functions not implemented for this platform"
#endif

	const U64 PageSize = GetPageSize();

#ifdef REX_CORE_TRACK_ALLOCS
	class NonTrackingMallocAllocator : public AllocatorBase<NonTrackingMallocAllocator>
	{
	public:
		[[nodiscard]] void* Allocate(U64 size, U64 alignment)
		{
			return _aligned_malloc(size, alignment);
		}

		[[nodiscard]] void* Reallocate(void* ptr, [[maybe_unused]] U64 oldSize, U64 newSize, U64 alignment)
		{
			REX_CORE_ASSERT(ptr != nullptr);
			return _aligned_realloc(ptr, newSize, alignment);
		}

		void Free(void* ptr, [[maybe_unused]] U64 size)
		{
			_aligned_free(ptr);
		}
	};
	static_assert(IAllocator<NonTrackingMallocAllocator>);

	struct Alloc
	{
		U64 size;
		AllocSourceLocation loc;
	};

	static std::unique_ptr<std::unordered_map<void*, Alloc, std::hash<void*>, std::equal_to<void*>, StdAllocatorAdaptor<std::pair<void* const, Alloc>, NonTrackingMallocAllocator>>> s_aliveAlloc;

	void TrackAlloc(void* ptr, U64 size, AllocSourceLocation loc)
	{
		if (!s_aliveAlloc)
			return;

		auto result = s_aliveAlloc->emplace(ptr, Alloc{ size, loc });
		if (result.second == false)
		{
			REX_CORE_ALLOC_NO_FREE(ptr, result.first->second.size, result.first->second.loc, size, loc);
		}
	}

	void TrackFree(void* ptr, U64 size, [[maybe_unused]]AllocSourceLocation loc)
	{
		if (!s_aliveAlloc)
			return;

		auto found = s_aliveAlloc->find(ptr);
		if (found == s_aliveAlloc->end())
		{
			REX_CORE_FREE_NO_ALLOC(ptr, size, loc);
			return;
		}
		else
		{
			if (found->second.size != size)
			{
				REX_CORE_ASYMMETRIC_FREE(ptr, size, loc, found->second.size, found->second.loc);
			}
			s_aliveAlloc->erase(ptr);
		}
	}

	void StartTrackingMemory()
	{
		s_aliveAlloc = std::make_unique<decltype(s_aliveAlloc)::element_type>();
	}

	bool CheckForLeaks()
	{
		REX_CORE_ASSERT(s_aliveAlloc, "RexCore::StartTrackingMemory() was not called");

		for (auto&[ptr, alloc] : *s_aliveAlloc)
		{
			REX_CORE_LEAK(ptr, alloc.size, alloc.loc);
		}

		const bool leaks = !s_aliveAlloc->empty();
		s_aliveAlloc.reset();

		return leaks;
	}
#endif
}
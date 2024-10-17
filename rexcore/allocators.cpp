#include <rexcore/allocators.hpp>
#include <rexcore/system_headers.hpp>

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

	void CommitPages(void* address, U64 numPages)
	{
#if (_MSC_VER <= 1900) || WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
		void* newPtr = VirtualAlloc(address, numPages * PageSize, MEM_COMMIT, PAGE_READWRITE);
#else
		void* newPtr = VirtualAllocFromApp(address, numPages * PageSize, MEM_COMMIT, PAGE_READWRITE);
#endif
		REX_CORE_ASSERT(newPtr != nullptr && address == newPtr);
	}

	void DecommitPages(void* address, U64 numPages)
	{
		REX_CORE_ASSERT(VirtualFree(address, numPages * PageSize, MEM_DECOMMIT));
	}


#else
#error "Page allocation functions not implemented for this platform"
#endif

	const U64 PageSize = GetPageSize();
}
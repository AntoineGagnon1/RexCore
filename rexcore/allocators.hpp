#pragma once

#include <rexcore/types.hpp>
#include <rexcore/config.hpp>
#include <rexcore/system_headers.hpp>
#include <rexcore/math.hpp>

#include <cstring> // std::memcpy and std::memmove
#include <cstdlib>
#include <concepts>

namespace RexCore
{
	inline void MemCopy(void* source, void* dest, U64 size)
	{
		std::memcpy(dest, source, size);
	}

	// Allows overlapping memory regions
	inline void MemMove(void* source, void* dest, U64 size)
	{
		std::memmove(dest, source, size);
	}

	extern const U64 PageSize;
	void* ReservePages(U64 numPages);
	void ReleasePages(void* address, U64 numPages);
	void CommitPages(void* address, U64 numPages);
	void DecommitPages(void* address, U64 numPages);

	template<typename T>
	concept Allocator = std::movable<T> && std::default_initializable<T>&& requires(T a)
	{
		{ a.Allocate(U64{}, U64{}) } -> std::convertible_to<void*>;
		{ a.Reallocate(nullptr, U64{}, U64{}, U64{}) } -> std::convertible_to<void*>;
		{ a.Free(nullptr, U64{}) } -> std::convertible_to<void>;
	};

	class AllocatorBase
	{
	public:
		[[nodiscard]] void* Reallocate(this auto&& self, void* ptr, U64 oldSize, U64 newSize, U64 alignment)
		{
			static_assert(Allocator<std::remove_reference_t<decltype(self)>>);
			REX_CORE_ASSERT(ptr != nullptr);
			void* newPtr = self.Allocate(newSize, alignment);
			MemMove(ptr, newPtr, oldSize);
			self.Free(ptr, oldSize);
			return newPtr;
		}
	};


	class MallocAllocator : public AllocatorBase
	{
	public:
		[[nodiscard]] void* Allocate(U64 size, U64 alignment)
		{
			return _aligned_malloc(size, alignment);
		}

		[[nodiscard]]void* Reallocate(void* ptr, [[maybe_unused]] U64 oldSize, U64 newSize, U64 alignment)
		{
			REX_CORE_ASSERT(ptr != nullptr);
			return _aligned_realloc(ptr, newSize, alignment);
		}

		void Free(void* ptr, [[maybe_unused]] U64 size)
		{
			_aligned_free(ptr);
		}
	};
	static_assert(Allocator<MallocAllocator>);

	// Allocations will always be page aligned, passing an alignment greater than the page size will assert
	class PageAllocator : public AllocatorBase
	{
	public:
		[[nodiscard]] void* Allocate(U64 size, U64 alignment)
		{
			REX_CORE_ASSERT(alignment <= PageSize);
			const U64 numPages = Math::CeilDiv(size, PageSize);
			void* pages = ReservePages(numPages);
			CommitPages(pages, numPages);
			return pages;
		}

		void Free(void* ptr, [[maybe_unused]] U64 size)
		{
			const U64 numPages = Math::CeilDiv(size, PageSize);
			DecommitPages(ptr, numPages);
			ReleasePages(ptr, numPages);
		}
	};
	static_assert(Allocator<PageAllocator>);
}
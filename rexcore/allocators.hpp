#pragma once

#include <rexcore/core.hpp>
#include <rexcore/config.hpp>
#include <rexcore/system_headers.hpp>
#include <rexcore/math.hpp>

#include <cstring> // std::memcpy and std::memmove
#include <cstdlib>
#include <concepts>
#include <bit>

namespace RexCore
{
	inline void MemCopy(const void* source, void* dest, U64 size)
	{
		std::memcpy(dest, source, size);
	}

	// Allows overlapping memory regions
	inline void MemMove(void* source, void* dest, U64 size)
	{
		std::memmove(dest, source, size);
	}

	inline void MemSet(void* dest, U8 value, U64 size)
	{
		std::memset(dest, value, size);
	}

	inline constexpr bool IsBigEndian()
	{
		return std::endian::native == std::endian::big;
	}

	inline constexpr bool IsLittleEndian()
	{
		return std::endian::native == std::endian::little;
	}

	extern const U64 PageSize;
	void* ReservePages(U64 numPages);
	void ReleasePages(void* address, U64 numPages);
	void CommitPages(void* address, U64 numPages);
	void DecommitPages(void* address, U64 numPages);

	template<typename T>
	concept IAllocator = std::movable<T> && std::default_initializable<T>&& requires(T a)
	{
		{ a.Allocate(U64{}, U64{}) } -> std::convertible_to<void*>;
		{ a.Reallocate(nullptr, U64{}, U64{}, U64{}) } -> std::convertible_to<void*>;
		{ a.Free(nullptr, U64{}) } -> std::convertible_to<void>;
	};

	template<typename T>
	class AllocatorBase
	{
	public:
		using AllocatorType = T;

		[[nodiscard]] void* Reallocate(this auto&& self, void* ptr, U64 oldSize, U64 newSize, U64 alignment)
		{
			static_assert(IAllocator<std::remove_reference_t<decltype(self)>>);
			REX_CORE_ASSERT(ptr != nullptr);
			void* newPtr = self.Allocate(newSize, alignment);
			MemMove(ptr, newPtr, oldSize);
			self.Free(ptr, oldSize);
			return newPtr;
		}
	};


	class MallocAllocator : public AllocatorBase<MallocAllocator>
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
	static_assert(IAllocator<MallocAllocator>);

	// Allocations will always be page aligned, passing an alignment greater than the page size will assert
	class PageAllocator : public AllocatorBase<PageAllocator>
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
	static_assert(IAllocator<PageAllocator>);

	using DefaultAllocator = MallocAllocator;
}
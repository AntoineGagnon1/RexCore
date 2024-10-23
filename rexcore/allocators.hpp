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

	inline U64 AlignedOffset(const void* address, U64 align)
	{
		const U64 intAddr = reinterpret_cast<U64>(address);
#pragma warning(suppress: 4146)
		const U64 aligned = (intAddr - 1u + align) & -align;
		return aligned - intAddr;
	}

	extern const U64 PageSize;
	void* ReservePages(U64 numPages);
	void ReleasePages(void* address, U64 numPages);
	void CommitPages(void* address, U64 numPages);
	void DecommitPages(void* address, U64 numPages);

	template<typename T>
	concept IAllocator = std::movable<T> && requires()
	{
		{ std::declval<T>().Allocate(U64{}, U64{}) } -> std::convertible_to<void*>;
		{ std::declval<T>().Reallocate(nullptr, U64{}, U64{}, U64{}) } -> std::convertible_to<void*>;
		{ std::declval<T>().Free(nullptr, U64{}) } -> std::convertible_to<void>;
	};

	template<IAllocator Allocator>
	using AllocatorRef = std::conditional_t<std::is_empty_v<Allocator>, Allocator, std::add_lvalue_reference_t<Allocator>>;

	template<IAllocator Allocator>
	inline consteval AllocatorRef<Allocator> AllocatorRefDefaultArg()
	{
		static_assert(std::is_empty_v<Allocator>, "You must pass an allocator reference if the allocator is not stateless");
		return Allocator{};
	};

	template<typename T>
	class AllocatorBase
	{
	public:
		using AllocatorType = T;

		// alignment must be the same as the original allocation
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

	class ArenaAllocator : public AllocatorBase<ArenaAllocator>
	{
	public:
		explicit ArenaAllocator(U64 maxSize = 16llu * 1024llu * 1024llu * 1024llu)
			: m_currentSize(0)
			, m_commitedSize(0)
			, m_maxSize(maxSize)
		{
			m_data = static_cast<U8*>(ReservePages(Math::CeilDiv(maxSize, PageSize)));
		}

		~ArenaAllocator()
		{
			DecommitPages(m_data, Math::CeilDiv(m_commitedSize, PageSize));
			ReleasePages(m_data, Math::CeilDiv(m_maxSize, PageSize));
			m_data = nullptr;
			m_maxSize = 0;
			m_currentSize = 0;
			m_commitedSize = 0;
		}

		[[nodiscard]] void* Allocate(U64 size, U64 alignment)
		{
			const U64 offset = AlignedOffset(m_data + m_currentSize, alignment);
			const U64 startIndex = m_currentSize + offset;
			m_currentSize = startIndex + size;

			CommitNewPages();

			return m_data + startIndex;
		}

		[[nodiscard]] void* Reallocate(void* ptr, [[maybe_unused]] U64 oldSize, U64 newSize, U64 alignment)
		{
			REX_CORE_ASSERT(ptr != nullptr && ptr <= m_data + m_currentSize);
			
			if (ptr == m_data + m_currentSize - oldSize)
			{ // If we are reallocating the last allocation we can just extend the current allocation
				m_currentSize += newSize - oldSize;
				CommitNewPages();
				return ptr;
			}
			else
			{
				void* newPtr = Allocate(newSize, alignment);
				MemCopy(ptr, newPtr, oldSize);
				return newPtr;
			}
		}

		void Free([[maybe_unused]] void* ptr, [[maybe_unused]] U64 size)
		{
		}

		void Reset()
		{
			m_currentSize = 0;
		}

	private:
		void CommitNewPages()
		{
			if (m_currentSize > m_commitedSize)
			{
				REX_CORE_ASSERT(m_currentSize < m_maxSize);
				const U64 numPages = Math::CeilDiv(m_currentSize - m_commitedSize, PageSize);
				CommitPages(m_data + m_commitedSize, numPages);
				m_commitedSize += numPages * PageSize;
			}
		}

	private:
		U8* m_data;
		U64 m_maxSize;
		U64 m_currentSize;
		U64 m_commitedSize;
	};
	static_assert(IAllocator<ArenaAllocator>);

	using DefaultAllocator = MallocAllocator;
}
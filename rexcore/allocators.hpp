#pragma once

#include <rexcore/core.hpp>
#include <rexcore/config.hpp>
#include <rexcore/system_headers.hpp>
#include <rexcore/math.hpp>

#include <cstring> // std::memcpy and std::memmove
#include <cstdlib>
#include <concepts>
#include <bit>
#include <source_location>
#include <functional>

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

#ifdef REX_CORE_TRACK_ALLOCS
	using AllocSourceLocation = std::source_location;

	void TrackAlloc(void* ptr, U64 size, AllocSourceLocation loc);
	void TrackFree(void* ptr, U64 size, AllocSourceLocation loc);
	
	void StartTrackingMemory();
	// Will call REX_CORE_LEAK for each leaked allocation
	// returns true if any leaks were detected
	bool CheckForLeaks();
#else
	struct AllocSourceLocation
	{
		static AllocSourceLocation current() noexcept { return {}; }
	};

	inline void TrackAlloc([[maybe_unused]] void* ptr, [[maybe_unused]] U64 size, [[maybe_unused]] AllocSourceLocation loc) {}
	inline void TrackFree([[maybe_unused]] void* ptr, [[maybe_unused]] U64 size, [[maybe_unused]] AllocSourceLocation loc) {}
	void StartTrackingMemory() {}
	inline bool CheckForLeaks() { return false; }
#endif

	extern const U64 PageSize;
	void* ReservePages(U64 numPages);
	void ReleasePages(void* address, U64 numPages);
	void CommitPagesUntracked(void* address, U64 numPages);
	inline void CommitPages(void* address, U64 numPages, AllocSourceLocation loc = AllocSourceLocation::current())
	{
		CommitPagesUntracked(address, numPages);
		TrackAlloc(address, numPages * PageSize, loc);
	}
	void DecommitPagesUntracked(void* address, U64 numPages);
	inline void DecommitPages(void* address, U64 numPages, AllocSourceLocation loc = AllocSourceLocation::current())
	{
		DecommitPagesUntracked(address, numPages);
		TrackFree(address, numPages * PageSize, loc);
	}

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

		// alignment must be the same as the original allocation, newSize can be smaller than oldSize
		[[nodiscard]] void* Reallocate(this auto&& self, void* ptr, U64 oldSize, U64 newSize, U64 alignment, AllocSourceLocation loc = AllocSourceLocation::current())
		{
			static_assert(IAllocator<std::remove_reference_t<decltype(self)>>);
			REX_CORE_ASSERT(ptr != nullptr);
			void* newPtr = self.Allocate(newSize, alignment, loc);
			MemMove(ptr, newPtr, oldSize);
			self.Free(ptr, oldSize, loc);
			return newPtr;
		}
	};


	class MallocAllocator : public AllocatorBase<MallocAllocator>
	{
	public:
		[[nodiscard]] void* Allocate(U64 size, U64 alignment, AllocSourceLocation loc = AllocSourceLocation::current())
		{
			void* ptr = _aligned_malloc(size, alignment);
			TrackAlloc(ptr, size, loc);
			return ptr;
		}

		[[nodiscard]]void* Reallocate(void* ptr, [[maybe_unused]] U64 oldSize, U64 newSize, U64 alignment, AllocSourceLocation loc = AllocSourceLocation::current())
		{
			REX_CORE_ASSERT(ptr != nullptr);
			TrackFree(ptr, oldSize, loc);
			void* newPtr = _aligned_realloc(ptr, newSize, alignment);
			TrackAlloc(newPtr, newSize, loc);
			return newPtr;
		}

		void Free(void* ptr, [[maybe_unused]] U64 size, AllocSourceLocation loc = AllocSourceLocation::current())
		{
			TrackFree(ptr, size, loc);
			_aligned_free(ptr);
		}
	};
	static_assert(IAllocator<MallocAllocator>);

	// Allocations will always be page aligned, passing an alignment greater than the page size will assert
	class PageAllocator : public AllocatorBase<PageAllocator>
	{
	public:
		[[nodiscard]] void* Allocate(U64 size, U64 alignment, AllocSourceLocation loc = AllocSourceLocation::current())
		{
			REX_CORE_ASSERT(alignment <= PageSize);
			const U64 numPages = Math::CeilDiv(size, PageSize);
			void* pages = ReservePages(numPages);
			CommitPagesUntracked(pages, numPages);
			TrackAlloc(pages, size, loc);
			return pages;
		}

		void Free(void* ptr, [[maybe_unused]] U64 size, AllocSourceLocation loc = AllocSourceLocation::current())
		{
			const U64 numPages = Math::CeilDiv(size, PageSize);
			DecommitPagesUntracked(ptr, numPages);
			ReleasePages(ptr, numPages);
			TrackFree(ptr, size, loc);
		}
	};
	static_assert(IAllocator<PageAllocator>);

	class ArenaAllocator : public AllocatorBase<ArenaAllocator>
	{
	public:
		REX_CORE_NO_COPY(ArenaAllocator);
		REX_CORE_DEFAULT_MOVE(ArenaAllocator);

		explicit ArenaAllocator(U64 maxSize = 16llu * 1024llu * 1024llu * 1024llu)
			: m_currentSize(0)
			, m_commitedSize(0)
			, m_maxSize(maxSize)
		{
			m_data = static_cast<U8*>(ReservePages(Math::CeilDiv(maxSize, PageSize)));
		}

		~ArenaAllocator()
		{
			DecommitPagesUntracked(m_data, Math::CeilDiv(m_commitedSize, PageSize));
			ReleasePages(m_data, Math::CeilDiv(m_maxSize, PageSize));
			m_data = nullptr;
			m_maxSize = 0;
			m_currentSize = 0;
			m_commitedSize = 0;
		}

		[[nodiscard]] void* Allocate(U64 size, U64 alignment, [[maybe_unused]] AllocSourceLocation loc = AllocSourceLocation::current())
		{
			const U64 offset = AlignedOffset(m_data + m_currentSize, alignment);
			const U64 startIndex = m_currentSize + offset;
			m_currentSize = startIndex + size;

			CommitNewPages();

			return m_data + startIndex;
		}

		[[nodiscard]] void* Reallocate(void* ptr, [[maybe_unused]] U64 oldSize, U64 newSize, U64 alignment, [[maybe_unused]] AllocSourceLocation loc = AllocSourceLocation::current())
		{
			REX_CORE_ASSERT(ptr != nullptr && ptr <= m_data + m_currentSize);
			
			if (ptr == m_data + m_currentSize - oldSize)
			{ // If we are reallocating the last allocation we can just extend the current allocation
				m_currentSize += newSize - oldSize; // TODO : maybe uncommit pages when newSize < oldSize ?
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

		void Free([[maybe_unused]] void* ptr, [[maybe_unused]] U64 size, [[maybe_unused]] AllocSourceLocation loc = AllocSourceLocation::current())
		{
		}

		void Reset()
		{
			// TODO : maybe uncommit pages ?
			m_currentSize = 0;
		}

	private:
		void CommitNewPages()
		{
			if (m_currentSize > m_commitedSize)
			{
				REX_CORE_ASSERT(m_currentSize < m_maxSize);
				const U64 numPages = Math::CeilDiv(m_currentSize - m_commitedSize, PageSize);
				CommitPagesUntracked(m_data + m_commitedSize, numPages);
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

	template<typename T, IAllocator Allocator>
	class StdAllocatorAdaptor
	{
	public:
		using value_type = T;

		StdAllocatorAdaptor(AllocatorRef<Allocator> allocator = AllocatorRefDefaultArg<Allocator>()) noexcept
			: m_allocator(allocator)
		{}

		template<typename U>
		StdAllocatorAdaptor(const StdAllocatorAdaptor<U, Allocator>& other) noexcept
			: m_allocator(other.GetAllocator())
		{}

		[[nodiscard]] constexpr T* allocate(std::size_t num)
		{
			return reinterpret_cast<T*>(m_allocator.Allocate(static_cast<U64>(num) * sizeof(T), alignof(T)));
		}

		constexpr void deallocate(T* ptr, std::size_t num) noexcept
		{
			m_allocator.Free(ptr, static_cast<U64>(num) * sizeof(T));
		}

		std::size_t MaxAllocationSize() const noexcept
		{
			return Math::MaxValue<U64>();
		}

		bool operator==(const StdAllocatorAdaptor<T, Allocator>& rhs) const noexcept
		{
			if constexpr (std::is_empty_v<Allocator>)
			{
				return true;
			}
			else
			{
				return &m_allocator == &rhs.m_allocator;
			}
		}

		bool operator!=(const StdAllocatorAdaptor<T, Allocator>& rhs) const noexcept
		{
			return !(*this == rhs);
		}

		AllocatorRef<Allocator> GetAllocator() const noexcept
		{
			return m_allocator;
		}

	private:
		[[no_unique_address]] AllocatorRef<Allocator> m_allocator;
	};

	using DefaultAllocator = MallocAllocator;
}
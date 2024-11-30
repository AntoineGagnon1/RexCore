#pragma once

#include <rexcore/core.hpp>
#include <rexcore/config.hpp>
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
	// Pass size = 0 if the size is not known
	void TrackFree(void* ptr, U64 size, AllocSourceLocation loc);
	
	void StartTrackingMemory();
	// Will call REX_CORE_LEAK for each leaked allocation
	// returns true if any leaks were detected
	bool CheckForLeaks();
#else
	struct AllocSourceLocation
	{
		static consteval AllocSourceLocation current() noexcept { return {}; }
	};

	inline void TrackAlloc([[maybe_unused]] void* ptr, [[maybe_unused]] U64 size, [[maybe_unused]] AllocSourceLocation loc) {}
	inline void TrackFree([[maybe_unused]] void* ptr, [[maybe_unused]] U64 size, [[maybe_unused]] AllocSourceLocation loc) {}
	inline void StartTrackingMemory() {}
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
		{ std::declval<T>().AllocateUntracked(U64{}, U64{}) } -> std::convertible_to<void*>;
		{ std::declval<T>().Reallocate(nullptr, U64{}, U64{}, U64{}) } -> std::convertible_to<void*>;
		{ std::declval<T>().ReallocateUntracked(nullptr, U64{}, U64{}, U64{}) } -> std::convertible_to<void*>;
		{ std::declval<T>().Free(nullptr, U64{}) } -> std::convertible_to<void>;
		{ std::declval<T>().FreeUntracked(nullptr, U64{}) } -> std::convertible_to<void>;
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

		[[nodiscard]] void* Allocate(this auto&& self, U64 size, U64 alignment, AllocSourceLocation loc = AllocSourceLocation::current())
		{
			void* ptr = self.AllocateUntracked(size, alignment);
			TrackAlloc(ptr, size, loc);
			return ptr;
		}

		// alignment must be the same as the original allocation, newSize can be smaller than oldSize
		[[nodiscard]] void* Reallocate(this auto&& self, void* ptr, U64 oldSize, U64 newSize, U64 alignment, AllocSourceLocation loc = AllocSourceLocation::current())
		{
			void* newPtr = self.ReallocateUntracked(ptr, oldSize, newSize, alignment);
			TrackFree(ptr, oldSize, loc);
			TrackAlloc(newPtr, newSize, loc);
			return newPtr;
		}

		// alignment must be the same as the original allocation, newSize can be smaller than oldSize
		[[nodiscard]] void* ReallocateUntracked(this auto&& self, void* ptr, U64 oldSize, U64 newSize, U64 alignment)
		{
			REX_CORE_TRACE_FUNC();
			static_assert(IAllocator<std::remove_reference_t<decltype(self)>>);
			REX_CORE_ASSERT(ptr != nullptr);
			void* newPtr = self.AllocateUntracked(newSize, alignment);
			MemMove(ptr, newPtr, oldSize);
			self.FreeUntracked(ptr, oldSize);
			return newPtr;
		}

		void Free(this auto&& self, void* ptr, U64 size, AllocSourceLocation loc = AllocSourceLocation::current())
		{
			self.FreeUntracked(ptr, size);
			TrackFree(ptr, size, loc);
		}
	};


	class MallocAllocator : public AllocatorBase<MallocAllocator>
	{
	public:
		[[nodiscard]] void* AllocateUntracked(U64 size, U64 alignment)
		{
			REX_CORE_TRACE_FUNC();
			return _aligned_malloc(size, alignment);
		}

		[[nodiscard]]void* ReallocateUntracked(void* ptr, [[maybe_unused]]U64 oldSize, U64 newSize, U64 alignment)
		{
			REX_CORE_TRACE_FUNC();
			REX_CORE_ASSERT(ptr != nullptr);
			return _aligned_realloc(ptr, newSize, alignment);
		}

		void FreeUntracked(void* ptr, [[maybe_unused]] U64 size)
		{
			REX_CORE_TRACE_FUNC();
			_aligned_free(ptr);
		}

		void FreeNoSize(void* ptr, AllocSourceLocation loc = AllocSourceLocation::current())
		{
			REX_CORE_TRACE_FUNC();
			Free(ptr, 0, loc);
		}
	};
	static_assert(IAllocator<MallocAllocator>);

	// Allocations will always be page aligned, passing an alignment greater than the page size will assert
	class PageAllocator : public AllocatorBase<PageAllocator>
	{
	public:
		[[nodiscard]] void* AllocateUntracked(U64 size, U64 alignment)
		{
			REX_CORE_TRACE_FUNC();
			REX_CORE_ASSERT(alignment <= PageSize);
			const U64 numPages = Math::CeilDiv(size, PageSize);
			void* pages = ReservePages(numPages);
			CommitPagesUntracked(pages, numPages);
			return pages;
		}

		void FreeUntracked(void* ptr, [[maybe_unused]] U64 size)
		{
			REX_CORE_TRACE_FUNC();
			const U64 numPages = Math::CeilDiv(size, PageSize);
			DecommitPagesUntracked(ptr, numPages);
			ReleasePages(ptr, numPages);
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

		[[nodiscard]] void* AllocateUntracked(U64 size, U64 alignment)
		{
			REX_CORE_TRACE_FUNC();
			const U64 offset = AlignedOffset(m_data + m_currentSize, alignment);
			const U64 startIndex = m_currentSize + offset;
			m_currentSize = startIndex + size;

			CommitNewPages();

			return m_data + startIndex;
		}

		[[nodiscard]] void* Allocate(U64 size, U64 alignment, [[maybe_unused]] AllocSourceLocation loc = AllocSourceLocation::current())
		{
			return AllocateUntracked(size, alignment);
		}

		[[nodiscard]] void* ReallocateUntracked(void* ptr, [[maybe_unused]] U64 oldSize, U64 newSize, U64 alignment)
		{
			REX_CORE_TRACE_FUNC();
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

		[[nodiscard]] void* Reallocate(void* ptr, [[maybe_unused]] U64 oldSize, U64 newSize, U64 alignment, [[maybe_unused]] AllocSourceLocation loc = AllocSourceLocation::current())
		{
			return ReallocateUntracked(ptr, oldSize, newSize, alignment);
		}

		void FreeUntracked([[maybe_unused]] void* ptr, [[maybe_unused]] U64 size)
		{
		}

		void Free([[maybe_unused]] void* ptr, [[maybe_unused]] U64 size, [[maybe_unused]] AllocSourceLocation loc = AllocSourceLocation::current())
		{
		}

		void Reset()
		{
			REX_CORE_TRACE_FUNC();
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

	// Fast allocator for fixed size items
	template<U64 ChunkSize, U64 Alignment = alignof(std::max_align_t), IAllocator ChunkAllocator = MallocAllocator>
	class PoolAllocatorBase : public AllocatorBase<PoolAllocatorBase<ChunkSize, Alignment, ChunkAllocator>>
	{
	public:
		static_assert(ChunkSize >= sizeof(void*), "ChunkSize must be at least as big as a pointer");
		static_assert(Alignment % alignof(void*) == 0, "Alignment must be a multiple of pointer alignment");

		REX_CORE_NO_COPY(PoolAllocatorBase);
		REX_CORE_DEFAULT_MOVE(PoolAllocatorBase);

		constexpr explicit PoolAllocatorBase(AllocatorRef<ChunkAllocator> allocator = AllocatorRefDefaultArg<ChunkAllocator>()) noexcept
			: m_allocator(allocator)
		{}

		constexpr ~PoolAllocatorBase() 
		{
			Chunk* chunk = m_freeList;
			while (chunk != nullptr)
			{
				Chunk* next = chunk->nextFree;
				m_allocator.FreeUntracked(chunk, sizeof(Chunk));
				chunk = next;
			}
		}

		// [size] must always be ChunkSize
		// [alignment] must always be Alignment
		[[nodiscard]] void* AllocateUntracked(U64 size, U64 alignment)
		{
			REX_CORE_TRACE_FUNC();
			REX_CORE_ASSERT(size == ChunkSize && alignment == Alignment);

			if (m_freeList != nullptr)
			{
				Chunk* chunk = m_freeList;
				m_freeList = chunk->nextFree;
				return chunk;
			}
			else
			{
				return m_allocator.AllocateUntracked(sizeof(Chunk), Alignment);
			}
		}

		// [size] must always be ChunkSize
		void FreeUntracked(void* ptr, U64 size)
		{
			REX_CORE_TRACE_FUNC();
			REX_CORE_ASSERT(size == ChunkSize);

			if (ptr == nullptr) {
				return;
			}

			Chunk* chunk = static_cast<Chunk*>(ptr);
			chunk->nextFree = m_freeList;
			m_freeList = chunk;
		}

	private:
		struct Chunk {
			union {
				Byte data[ChunkSize];
				Chunk* nextFree;
			};
		};

		[[no_unique_address]] AllocatorRef<ChunkAllocator> m_allocator;
		Chunk* m_freeList = nullptr;
	};
	static_assert(IAllocator<PoolAllocatorBase<32>>);

	template<typename T, IAllocator ChunkAllocator = MallocAllocator>
	class PoolAllocator : public AllocatorBase<PoolAllocator<T, ChunkAllocator>>
	{
	public:
		static_assert(sizeof(T) >= sizeof(void*), "T must be at least as big as a pointer");
		static_assert(alignof(T) % alignof(void*) == 0 || alignof(void*) % alignof(T) == 0, "T's alignment must be a multiple of pointer alignment");

		REX_CORE_NO_COPY(PoolAllocator);
		REX_CORE_DEFAULT_MOVE(PoolAllocator);

		constexpr explicit PoolAllocator(AllocatorRef<ChunkAllocator> allocator = AllocatorRefDefaultArg<ChunkAllocator>()) noexcept
			: m_pool(allocator)
		{}

		// [size] must always be sizeof(T)
		// [alignment] must always be Max(alignof(T), alignof(std::max_align_t))
		// Do not call this function directly, use the overload that returns a T* instead
		[[nodiscard]] void* AllocateUntracked(U64 size, U64 alignment)
		{
			return m_pool.AllocateUntracked(size, alignment);
		}

		[[nodiscard]] T* AllocateItem(AllocSourceLocation loc = AllocSourceLocation::current())
		{
			return static_cast<T*>(AllocatorBase<PoolAllocator<T, ChunkAllocator>>::Allocate(ChunkSize, Alignment, loc));
		}

		// [size] must always be sizeof(T)
		// Dot not call this function directly, use the overload that takes a T* instead
		void FreeUntracked(void* ptr, U64 size)
		{
			m_pool.FreeUntracked(ptr, size);
		}

		void FreeItem(T* ptr, AllocSourceLocation loc = AllocSourceLocation::current())
		{
			AllocatorBase<PoolAllocator<T, ChunkAllocator>>::Free(static_cast<void*>(ptr), ChunkSize, loc);
		}

	private:
		constexpr static U64 Alignment = Math::Max(alignof(T), alignof(std::max_align_t));
		constexpr static U64 ChunkSize = sizeof(T);

		PoolAllocatorBase<ChunkSize, Alignment, ChunkAllocator> m_pool;
	};
	static_assert(IAllocator<PoolAllocator<U64>>);

	template<typename T, IAllocator Allocator>
	class StdAllocatorAdaptor : public AllocatorRef<Allocator>
	{
	public:
		using value_type = T;

		template<typename U>
		struct rebind {
			using other = StdAllocatorAdaptor<U, Allocator>;
		};

		StdAllocatorAdaptor(AllocatorRef<Allocator> allocator = AllocatorRefDefaultArg<Allocator>()) noexcept
			: AllocatorRef<Allocator>(allocator)
		{}

		template<typename U>
		StdAllocatorAdaptor(const StdAllocatorAdaptor<U, Allocator>& other) noexcept
			: AllocatorRef<Allocator>(other.GetAllocator())
		{}

		[[nodiscard]] constexpr T* allocate(std::size_t num)
		{
			return reinterpret_cast<T*>(AllocatorRef<Allocator>::Allocate(static_cast<U64>(num) * sizeof(T), alignof(T)));
		}

		constexpr void deallocate(T* ptr, std::size_t num) noexcept
		{
			AllocatorRef<Allocator>::Free(ptr, static_cast<U64>(num) * sizeof(T));
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
				return &GetAllocator() == &rhs.GetAllocator();
			}
		}

		bool operator!=(const StdAllocatorAdaptor<T, Allocator>& rhs) const noexcept
		{
			return !(*this == rhs);
		}

		AllocatorRef<Allocator> GetAllocator() const noexcept
		{
			return *static_cast<const AllocatorRef<Allocator>*>(this);
		}
	};

	template<IAllocator Allocator>
	class NonTracking : public Allocator
	{
	public:
		[[nodiscard]] void* Allocate(U64 size, U64 alignment)
		{
			return Allocator::AllocateUntracked(size, alignment);
		}

		[[nodiscard]] void* Reallocate(void* ptr, U64 oldSize, U64 newSize, U64 alignment)
		{
			return Allocator::ReallocateUntracked(ptr, oldSize, newSize, alignment);
		}

		void Free(void* ptr, U64 size)
		{
			Allocator::FreeUntracked(ptr, size);
		}
	};
	static_assert(IAllocator<NonTracking<MallocAllocator>>);

	using DefaultAllocator = MallocAllocator;
	using DefaultNonTrackingAllocator = NonTracking<MallocAllocator>;
}

#ifndef REX_CORE_TRACK_ALLOCS
template <>
struct std::formatter<RexCore::AllocSourceLocation> {
	constexpr auto parse(std::format_parse_context& ctx) const {
		return ctx.begin();
	}

	template <typename FormatContext>
	auto format(const RexCore::AllocSourceLocation& loc, FormatContext& ctx) const {
		return std::format_to(ctx.out(), "(Optimized AllocSourceLocation)");
	}
};
#endif
#pragma once
#include <rexcore/core.hpp>
#include <rexcore/allocators.hpp>
#include <rexcore/concepts.hpp>
#include <rexcore/containers/function.hpp>

namespace RexCore
{
	template<typename T, IAllocator Allocator = DefaultAllocator>
	class UniquePtr
	{
	public:
		using ValueType = T;
		using PtrType = T*;

		REX_CORE_NO_COPY(UniquePtr);
		
		template<typename T2, IAllocator Allocator2>
		constexpr UniquePtr(UniquePtr<T2, Allocator2>&& other) noexcept
			: m_ptr(static_cast<T*>(other.m_ptr)), m_allocator(other.m_allocator)
		{
			other.m_ptr = nullptr;
		}

		template<typename T2, IAllocator Allocator2>
		constexpr UniquePtr& operator=(UniquePtr<T2, Allocator2>&& other) noexcept
		{
			if (this != &other)
			{
				Free();
				m_ptr = static_cast<T*>(other.m_ptr);
				m_allocator = other.m_allocator;
				other.m_ptr = nullptr;
			}
			return *this;
		}

		explicit constexpr UniquePtr(AllocatorRef<Allocator> allocator = AllocatorRefDefaultArg<Allocator>())
			: m_allocator(allocator)
			, m_ptr(nullptr)
		{
		}

		// Warning : ptr must have been allocated with the same allocator
		explicit constexpr UniquePtr(PtrType ptr, AllocatorRef<Allocator> allocator = AllocatorRefDefaultArg<Allocator>())
			: m_allocator(allocator)
			, m_ptr(ptr)
		{
		}

		constexpr ~UniquePtr()
		{
			Free();
		}

		[[nodiscard]] constexpr bool IsEmpty() const { return m_ptr == nullptr; }

		[[nodiscard]] constexpr UniquePtr Clone() const
		{
			static_assert(IClonable<T>, "The inner type must be IClonable in order to clone a UniquePtr");
			if (m_ptr == nullptr)
				return UniquePtr(nullptr, m_allocator);
			else
			{
				T* clonePtr = (T*)GetAllocator().Allocate(sizeof(T), alignof(T));
				RexCore::CloneInto(*m_ptr, clonePtr);
				return UniquePtr(clonePtr, m_allocator);
			}
		}

		constexpr void Free()
		{
			if (m_ptr)
			{
				m_allocator.Free(m_ptr, sizeof(T));
				m_ptr = nullptr;
			}
		}

		// Warning : ptr must have been allocated with the same allocator
		constexpr void Assign(PtrType ptr) noexcept
		{
			Free();
			m_ptr = ptr;
		}

		[[nodiscard]] constexpr AllocatorRef<Allocator> GetAllocator() const noexcept { return m_allocator; }

		[[nodiscard]] constexpr const PtrType Get() const noexcept { return m_ptr; }
		[[nodiscard]] constexpr PtrType Get() noexcept { return m_ptr; }

		[[nodiscard]] constexpr const PtrType operator->() const noexcept { return m_ptr; }
		[[nodiscard]] constexpr PtrType operator->() noexcept { return m_ptr; }

		[[nodiscard]] constexpr const T& operator*() const noexcept { return *m_ptr; }
		[[nodiscard]] constexpr T& operator*() noexcept { return *m_ptr; }

		[[nodiscard]] explicit constexpr operator bool() const noexcept { return m_ptr != nullptr; }

	private:
		template<typename T2, IAllocator Allocator>
		friend class UniquePtr;

		[[no_unique_address]] AllocatorRef<Allocator> m_allocator;
		PtrType m_ptr;
	};

	template<typename T, IAllocator Allocator, typename ...Args>
	constexpr UniquePtr<T, Allocator> AllocateUnique(AllocatorRef<Allocator> allocator, Args&&... args)
	{
		T* ptr = reinterpret_cast<T*>(allocator.Allocate(sizeof(T), alignof(T)));
		new (ptr) T(std::forward<Args>(args)...);
		return UniquePtr<T, Allocator>(ptr, allocator);
	}

	template<typename T, typename ...Args>
	constexpr UniquePtr<T, DefaultAllocator> MakeUnique(Args&&... args)
	{
		return AllocateUnique<T, DefaultAllocator, Args...>(DefaultAllocator{}, std::forward<Args>(args)...);
	}

	
	namespace Internal
	{
		struct SharedPtrControlBlock
		{
			U64 refCount;
			U64 weakRefCount; // + 1 for all the refCounts
			Function<void(void* objPtr, void* controlBlockPtr)> deallocate;

			constexpr SharedPtrControlBlock(U64 count, U64 weakCount, Function<void(void*, void*)> deallocate)
				: refCount(count), weakRefCount(weakCount), deallocate(std::move(deallocate))
			{}
		};
	}

	template<typename T>
	class SharedPtr;

	// NOT thread-safe
	template<typename T>
	class WeakPtr
	{
	public:
		using ValueType = T;
		using PtrType = T*;

		constexpr WeakPtr()
			: m_ptr(nullptr)
			, m_controlBlock(nullptr)
		{}

		constexpr WeakPtr(const WeakPtr<T>& other)
			: m_ptr(other.m_ptr)
			, m_controlBlock(other.m_controlBlock)
		{
			if (m_controlBlock)
				m_controlBlock->weakRefCount += 1;
		}

		constexpr WeakPtr(WeakPtr&& other) noexcept
			: m_ptr(other.m_ptr)
			, m_controlBlock(other.m_controlBlock)
		{
			other.m_ptr = nullptr;
			other.m_controlBlock = nullptr;
		}

		constexpr WeakPtr& operator=(WeakPtr&& other) noexcept
		{
			if (this != &other)
			{
				m_ptr = other.m_ptr;
				m_controlBlock = other.m_controlBlock;
				other.m_ptr = nullptr;
				other.m_controlBlock = nullptr;
			}
			return *this;
		}

		constexpr WeakPtr& operator=(const WeakPtr& other) noexcept
		{
			WeakPtr copy(other);
			Swap(copy);
			return *this;
		}

		constexpr void Swap(WeakPtr& other) noexcept
		{
			std::swap(m_ptr, other.m_ptr);
			std::swap(m_controlBlock, other.m_controlBlock);
		}

		constexpr ~WeakPtr()
		{
			if (m_controlBlock)
			{
				m_controlBlock->weakRefCount -= 1;

				if (m_controlBlock->weakRefCount == 0)
				{
					m_ptr->~T();
					m_controlBlock->deallocate(m_ptr, m_controlBlock);
				}
			}

			m_ptr = nullptr;
			m_controlBlock = nullptr;
		}


		[[nodiscard]] constexpr SharedPtr<T> Lock() const;

		[[nodiscard]] explicit constexpr operator bool() const noexcept { return !IsEmpty(); }

		[[nodiscard]] constexpr bool IsEmpty() const noexcept { return m_ptr == nullptr || m_controlBlock->refCount == 0; }
	private:
		constexpr WeakPtr(Internal::SharedPtrControlBlock* controlBlock, PtrType ptr)
			: m_controlBlock(controlBlock)
			, m_ptr(ptr)
		{
			if (m_controlBlock)
			{
				REX_CORE_ASSERT(m_ptr != nullptr);
				m_controlBlock->weakRefCount += 1;
			}
		}

		friend class SharedPtr<T>;

	private:
		Internal::SharedPtrControlBlock* m_controlBlock;
		PtrType m_ptr;
	};

	// NOT thread safe
	template<typename T>
	class SharedPtr
	{
	public:
		using ValueType = T;
		using PtrType = T*;

		constexpr SharedPtr(SharedPtr&& other) noexcept
			: m_controlBlock(other.m_controlBlock)
			, m_ptr(other.m_ptr)
		{
			other.m_controlBlock = nullptr;
			other.m_ptr = nullptr;
		}

		constexpr SharedPtr(const SharedPtr& other) noexcept
			: m_ptr(other.m_ptr)
			, m_controlBlock(other.m_controlBlock)
		{
			if (m_controlBlock)
				m_controlBlock->refCount += 1;
		}

		constexpr SharedPtr()
			: m_controlBlock(nullptr)
			, m_ptr(nullptr)
		{}

		constexpr ~SharedPtr()
		{
			Release();
		}

		constexpr SharedPtr& operator=(SharedPtr&& other) noexcept
		{
			if (this != &other)
			{
				m_controlBlock = other.m_controlBlock;
				m_ptr = other.m_ptr;
				other.m_controlBlock = nullptr;
				other.m_ptr = nullptr;
			}
			return *this;
		}

		constexpr SharedPtr& operator=(const SharedPtr& other) noexcept
		{
			SharedPtr copy(other);
			Swap(copy);
			return *this;
		}

		constexpr void Swap(SharedPtr& other)
		{
			std::swap(m_controlBlock, other.m_controlBlock);
			std::swap(m_ptr, other.m_ptr);
		}

		constexpr void Release()
		{
			if (m_controlBlock)
			{
				m_controlBlock->refCount -= 1;
				if (m_controlBlock->refCount == 0)
				{
					m_controlBlock->weakRefCount -= 1;

					if (m_controlBlock->weakRefCount == 0)
					{
						m_ptr->~T();
						m_controlBlock->deallocate(m_ptr, m_controlBlock);
					}
				}
			}

			m_ptr = nullptr;
			m_controlBlock = nullptr;
		}

		[[nodiscard]] constexpr bool IsEmpty() const { return m_ptr == nullptr; }

		// Warning : will make an extra allocation for the control block
		// Warning : ptr must have been allocated with [allocator]
		template<IAllocator Allocator>
		constexpr void Assign(PtrType ptr, AllocatorRef<Allocator> allocator = AllocatorRefDefaultArg<Allocator>()) noexcept
		{
			Release();
			
			m_ptr = ptr;
			if (m_ptr)
			{
				m_controlBlock = (Internal::SharedPtrControlBlock*)allocator.Allocate(sizeof(Internal::SharedPtrControlBlock), alignof(Internal::SharedPtrControlBlock));
				new (m_controlBlock) Internal::SharedPtrControlBlock(1, 1, [&allocator](void* tPtr, void* controlPtr) {
					allocator.Free(tPtr, sizeof(T));
					using ControlBlockType = Internal::SharedPtrControlBlock;
					((Internal::SharedPtrControlBlock*)controlPtr)->~ControlBlockType();
					allocator.Free(controlPtr, sizeof(Internal::SharedPtrControlBlock));
				});
			}
		}

		[[nodiscard]] constexpr WeakPtr<T> GetWeak() const
		{
			return WeakPtr<T>(m_controlBlock, m_ptr);
		}

		[[nodiscard]] constexpr const PtrType Get() const noexcept { return m_ptr; }
		[[nodiscard]] constexpr PtrType Get() noexcept { return m_ptr; }

		[[nodiscard]] constexpr const PtrType operator->() const noexcept { return m_ptr; }
		[[nodiscard]] constexpr PtrType operator->() noexcept { return m_ptr; }

		[[nodiscard]] constexpr const T& operator*() const noexcept { return *m_ptr; }
		[[nodiscard]] constexpr T& operator*() noexcept { return *m_ptr; }

		[[nodiscard]] explicit constexpr operator bool() const noexcept { return m_ptr != nullptr; }

		[[nodiscard]] constexpr U64 NumRefs() const noexcept { return m_controlBlock ? m_controlBlock->refCount : 0; }
		[[nodiscard]] constexpr U64 NumWeakRefs() const noexcept {
			if (!m_controlBlock)
				return 0;
			return m_controlBlock->refCount >= 1 ? m_controlBlock->weakRefCount - 1 : m_controlBlock->weakRefCount;
		}

	private:
		friend class WeakPtr<T>;

		template<typename T2, IAllocator Allocator, typename ...Args>
		friend constexpr SharedPtr<T2> AllocateShared(AllocatorRef<Allocator> allocator, Args&& ...args);

		constexpr SharedPtr(PtrType ptr, Internal::SharedPtrControlBlock* controlBlock)
			: m_controlBlock(controlBlock)
			, m_ptr(ptr)
		{
			if (m_controlBlock)
				m_controlBlock->refCount += 1;
		}

		Internal::SharedPtrControlBlock* m_controlBlock;
		PtrType m_ptr;
	};

	template<typename T>
	[[nodiscard]] constexpr SharedPtr<T> WeakPtr<T>::Lock() const
	{
		return SharedPtr<T>(m_ptr, m_controlBlock);
	}

	// Warning : will make an extra allocation for the control block
	// Warning : ptr must have been allocated with [allocator]
	template<typename T, IAllocator Allocator>
	[[nodiscard]] constexpr SharedPtr<T> MakeSharedFromPtr(T* ptr, AllocatorRef<Allocator> allocator = AllocatorRefDefaultArg<Allocator>())
	{
		SharedPtr<T> sharedPtr;
		sharedPtr.Assign<Allocator>(ptr, allocator);
		return sharedPtr;
	}

	template<typename T, IAllocator Allocator, typename ...Args>
	[[nodiscard]] constexpr SharedPtr<T> AllocateShared(AllocatorRef<Allocator> allocator, Args&& ...args)
	{
		struct DataAndControl
		{
			Internal::SharedPtrControlBlock controlBlock;
			T data;
		};

		DataAndControl* ptrDataAndControl = (DataAndControl*)allocator.Allocate(sizeof(DataAndControl), alignof(DataAndControl));

		// Yikes, can't control the type of the lambda capture so have one lambda by copy and one by reference :(
		// Would probably be better as a functor that takes a AllocatorRef<Allocator> as a member
		if constexpr (std::is_empty_v<Allocator>)
		{
			new (&ptrDataAndControl->controlBlock) Internal::SharedPtrControlBlock(0, 1, [allocator]([[maybe_unused]] void* tPtr, void* controlPtr) mutable {
				using ControlBlockType = Internal::SharedPtrControlBlock;
				((ControlBlockType*)controlPtr)->~ControlBlockType();
				allocator.Free(controlPtr, sizeof(DataAndControl));
			});
		}
		else
		{
			new (&ptrDataAndControl->controlBlock) Internal::SharedPtrControlBlock(0, 1, [&allocator]([[maybe_unused]] void* tPtr, void* controlPtr) mutable {
				using ControlBlockType = Internal::SharedPtrControlBlock;
				((ControlBlockType*)controlPtr)->~ControlBlockType();
				allocator.Free(controlPtr, sizeof(DataAndControl));
			});
		}

		new (&ptrDataAndControl->data) T(std::forward<Args>(args)...);

		return SharedPtr<T>((T*)&ptrDataAndControl->data, &ptrDataAndControl->controlBlock);
	}

	template<typename T, typename ...Args>
	[[nodiscard]] constexpr SharedPtr<T> MakeShared(Args&& ...args)
	{
		return AllocateShared<T, DefaultAllocator, Args...>(DefaultAllocator{}, std::forward<Args>(args)...);
	}

	namespace Internal
	{
		struct AtomicSharedPtrControlBlock
		{
			std::atomic_uint64_t refCount;
			std::atomic_uint64_t weakRefCount; // + 1 for all the refCounts
			Function<void(void* objPtr, void* controlBlockPtr)> deallocate;

			constexpr AtomicSharedPtrControlBlock(U64 count, U64 weakCount, Function<void(void*, void*)> deallocate)
				: refCount(count), weakRefCount(weakCount), deallocate(std::move(deallocate))
			{}
		};
	}

	template<typename T>
	class AtomicSharedPtr;

	// TODO perf : some of the atomic operations could be relaxed
	template<typename T>
	class AtomicWeakPtr
	{
	public:
		using ValueType = T;
		using PtrType = T*;

		constexpr AtomicWeakPtr()
			: m_ptr(nullptr)
			, m_controlBlock(nullptr)
		{}

		constexpr AtomicWeakPtr(const AtomicWeakPtr<T>& other)
			: m_ptr(other.m_ptr)
			, m_controlBlock(other.m_controlBlock)
		{
			if (m_controlBlock)
				m_controlBlock->weakRefCount += 1;
		}

		constexpr AtomicWeakPtr(AtomicWeakPtr&& other) noexcept
			: m_ptr(other.m_ptr)
			, m_controlBlock(other.m_controlBlock)
		{
			other.m_ptr = nullptr;
			other.m_controlBlock = nullptr;
		}

		constexpr AtomicWeakPtr& operator=(AtomicWeakPtr&& other) noexcept
		{
			if (this != &other)
			{
				m_ptr = other.m_ptr;
				m_controlBlock = other.m_controlBlock;
				other.m_ptr = nullptr;
				other.m_controlBlock = nullptr;
			}
			return *this;
		}

		constexpr AtomicWeakPtr& operator=(const AtomicWeakPtr& other) noexcept
		{
			AtomicWeakPtr copy(other);
			Swap(copy);
			return *this;
		}

		constexpr void Swap(AtomicWeakPtr& other) noexcept
		{
			std::swap(m_ptr, other.m_ptr);
			std::swap(m_controlBlock, other.m_controlBlock);
		}

		constexpr ~AtomicWeakPtr()
		{
			if (m_controlBlock)
			{

				if (--m_controlBlock->weakRefCount == 0)
				{
					m_ptr->~T();
					m_controlBlock->deallocate(m_ptr, m_controlBlock);
				}
			}

			m_ptr = nullptr;
			m_controlBlock = nullptr;
		}


		[[nodiscard]] constexpr AtomicSharedPtr<T> Lock() const;

		[[nodiscard]] explicit constexpr operator bool() const noexcept { return !IsEmpty(); }

		[[nodiscard]] constexpr bool IsEmpty() const noexcept { return m_ptr == nullptr || m_controlBlock->refCount == 0; }
	private:
		constexpr AtomicWeakPtr(Internal::AtomicSharedPtrControlBlock* controlBlock, PtrType ptr)
			: m_controlBlock(controlBlock)
			, m_ptr(ptr)
		{
			if (m_controlBlock)
			{
				REX_CORE_ASSERT(m_ptr != nullptr);
				m_controlBlock->weakRefCount++;
			}
		}

		friend class AtomicSharedPtr<T>;

	private:
		Internal::AtomicSharedPtrControlBlock* m_controlBlock;
		PtrType m_ptr;
	};

	// Thread safe
	template<typename T>
	class AtomicSharedPtr
	{
	public:
		using ValueType = T;
		using PtrType = T*;

		constexpr AtomicSharedPtr(AtomicSharedPtr&& other) noexcept
			: m_controlBlock(other.m_controlBlock)
			, m_ptr(other.m_ptr)
		{
			other.m_controlBlock = nullptr;
			other.m_ptr = nullptr;
		}

		constexpr AtomicSharedPtr(const AtomicSharedPtr& other) noexcept
			: m_ptr(other.m_ptr)
			, m_controlBlock(other.m_controlBlock)
		{
			if (m_controlBlock)
				m_controlBlock->refCount++;
		}

		constexpr AtomicSharedPtr()
			: m_controlBlock(nullptr)
			, m_ptr(nullptr)
		{}

		constexpr ~AtomicSharedPtr()
		{
			Release();
		}

		constexpr AtomicSharedPtr& operator=(AtomicSharedPtr&& other) noexcept
		{
			if (this != &other)
			{
				m_controlBlock = other.m_controlBlock;
				m_ptr = other.m_ptr;
				other.m_controlBlock = nullptr;
				other.m_ptr = nullptr;
			}
			return *this;
		}

		constexpr AtomicSharedPtr& operator=(const AtomicSharedPtr& other) noexcept
		{
			AtomicSharedPtr copy(other);
			Swap(copy);
			return *this;
		}

		constexpr void Swap(AtomicSharedPtr& other)
		{
			std::swap(m_controlBlock, other.m_controlBlock);
			std::swap(m_ptr, other.m_ptr);
		}

		constexpr void Release()
		{
			if (m_controlBlock)
			{
				if (--m_controlBlock->refCount == 0)
				{
					if (--m_controlBlock->weakRefCount == 0)
					{
						m_ptr->~T();
						m_controlBlock->deallocate(m_ptr, m_controlBlock);
					}
				}
			}

			m_ptr = nullptr;
			m_controlBlock = nullptr;
		}

		[[nodiscard]] constexpr bool IsEmpty() const { return m_ptr == nullptr; }

		// Warning : will make an extra allocation for the control block
		// Warning : ptr must have been allocated with [allocator]
		template<IAllocator Allocator>
		constexpr void Assign(PtrType ptr, AllocatorRef<Allocator> allocator = AllocatorRefDefaultArg<Allocator>()) noexcept
		{
			Release();

			m_ptr = ptr;
			if (m_ptr)
			{
				m_controlBlock = (Internal::AtomicSharedPtrControlBlock*)allocator.Allocate(sizeof(Internal::AtomicSharedPtrControlBlock), alignof(Internal::AtomicSharedPtrControlBlock));
				new (m_controlBlock) Internal::AtomicSharedPtrControlBlock(1, 1, [&allocator](void* tPtr, void* controlPtr) {
					allocator.Free(tPtr, sizeof(T));
					using ControlBlockType = Internal::AtomicSharedPtrControlBlock;
					((Internal::AtomicSharedPtrControlBlock*)controlPtr)->~ControlBlockType();
					allocator.Free(controlPtr, sizeof(Internal::AtomicSharedPtrControlBlock));
				});
			}
		}

		[[nodiscard]] constexpr AtomicWeakPtr<T> GetWeak() const
		{
			return AtomicWeakPtr<T>(m_controlBlock, m_ptr);
		}

		[[nodiscard]] constexpr const PtrType Get() const noexcept { return m_ptr; }
		[[nodiscard]] constexpr PtrType Get() noexcept { return m_ptr; }

		[[nodiscard]] constexpr const PtrType operator->() const noexcept { return m_ptr; }
		[[nodiscard]] constexpr PtrType operator->() noexcept { return m_ptr; }

		[[nodiscard]] constexpr const T& operator*() const noexcept { return *m_ptr; }
		[[nodiscard]] constexpr T& operator*() noexcept { return *m_ptr; }

		[[nodiscard]] explicit constexpr operator bool() const noexcept { return m_ptr != nullptr; }

		[[nodiscard]] constexpr U64 NumRefs() const noexcept { return m_controlBlock ? (U64)m_controlBlock->refCount : 0ull; }

		// Warning : this is not thread safe, don't depend on the result
		[[nodiscard]] constexpr U64 NumWeakRefs() const noexcept {
			if (!m_controlBlock)
				return 0;

			// If refCount is decremented after this line, but before the return line,
			// the weakRefCount won't be decremented, so the result will be off by 1
			const U64 weakRefCount = m_controlBlock->weakRefCount;
			return m_controlBlock->refCount >= 1 ? weakRefCount - 1 : weakRefCount;
		}

	private:
		friend class AtomicWeakPtr<T>;

		template<typename T2, IAllocator Allocator, typename ...Args>
		friend constexpr AtomicSharedPtr<T2> AllocateAtomicShared(AllocatorRef<Allocator> allocator, Args&& ...args);

		constexpr AtomicSharedPtr(PtrType ptr, Internal::AtomicSharedPtrControlBlock* controlBlock)
			: m_controlBlock(controlBlock)
			, m_ptr(ptr)
		{
			if (m_controlBlock)
				m_controlBlock->refCount++;
		}

		Internal::AtomicSharedPtrControlBlock* m_controlBlock;
		PtrType m_ptr;
	};

	template<typename T>
	[[nodiscard]] constexpr AtomicSharedPtr<T> AtomicWeakPtr<T>::Lock() const
	{
		return AtomicSharedPtr<T>(m_ptr, m_controlBlock);
	}

	// Warning : will make an extra allocation for the control block
	// Warning : ptr must have been allocated with [allocator]
	template<typename T, IAllocator Allocator>
	[[nodiscard]] constexpr AtomicSharedPtr<T> MakeAtomicSharedFromPtr(T* ptr, AllocatorRef<Allocator> allocator = AllocatorRefDefaultArg<Allocator>())
	{
		AtomicSharedPtr<T> sharedPtr;
		sharedPtr.Assign<Allocator>(ptr, allocator);
		return sharedPtr;
	}

	template<typename T, IAllocator Allocator, typename ...Args>
	[[nodiscard]] constexpr AtomicSharedPtr<T> AllocateAtomicShared(AllocatorRef<Allocator> allocator, Args&& ...args)
	{
		struct DataAndControl
		{
			Internal::AtomicSharedPtrControlBlock controlBlock;
			T data;
		};

		DataAndControl* ptrDataAndControl = (DataAndControl*)allocator.Allocate(sizeof(DataAndControl), alignof(DataAndControl));

		// Yikes, can't control the type of the lambda capture so have one lambda by copy and one by reference :(
		// Would probably be better as a functor that takes a AllocatorRef<Allocator> as a member
		if constexpr (std::is_empty_v<Allocator>)
		{
			new (&ptrDataAndControl->controlBlock) Internal::AtomicSharedPtrControlBlock(0, 1, [allocator]([[maybe_unused]] void* tPtr, void* controlPtr) mutable {
				using ControlBlockType = Internal::AtomicSharedPtrControlBlock;
				((ControlBlockType*)controlPtr)->~ControlBlockType();
				allocator.Free(controlPtr, sizeof(DataAndControl));
			});
		}
		else
		{
			new (&ptrDataAndControl->controlBlock) Internal::AtomicSharedPtrControlBlock(0, 1, [&allocator]([[maybe_unused]] void* tPtr, void* controlPtr) mutable {
				using ControlBlockType = Internal::AtomicSharedPtrControlBlock;
				((ControlBlockType*)controlPtr)->~ControlBlockType();
				allocator.Free(controlPtr, sizeof(DataAndControl));
			});
		}

		new (&ptrDataAndControl->data) T(std::forward<Args>(args)...);

		return AtomicSharedPtr<T>((T*)&ptrDataAndControl->data, &ptrDataAndControl->controlBlock);
	}

	template<typename T, typename ...Args>
	[[nodiscard]] constexpr AtomicSharedPtr<T> MakeAtomicShared(Args&& ...args)
	{
		return AllocateAtomicShared<T, DefaultAllocator, Args...>(DefaultAllocator{}, std::forward<Args>(args)...);
	}
}
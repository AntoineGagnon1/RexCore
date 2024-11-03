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
		
		UniquePtr(UniquePtr&& other) noexcept
			: m_ptr(other.m_ptr)
		{
			other.m_ptr = nullptr;
		}

		UniquePtr& operator=(UniquePtr&& other) noexcept
		{
			if (this != &other)
			{
				m_ptr = other.m_ptr;
				other.m_ptr = nullptr;
			}
			return *this;
		}

		explicit UniquePtr(AllocatorRef<Allocator> allocator = AllocatorRefDefaultArg<Allocator>())
			: m_allocator(allocator)
			, m_ptr(nullptr)
		{
		}

		// Warning : ptr must have been allocated with the same allocator
		explicit UniquePtr(PtrType ptr, AllocatorRef<Allocator> allocator = AllocatorRefDefaultArg<Allocator>())
			: m_allocator(allocator)
			, m_ptr(ptr)
		{
		}

		~UniquePtr()
		{
			Free();
		}

		[[nodiscard]] bool IsEmpty() const { return m_ptr == nullptr; }

		[[nodiscard]] UniquePtr Clone() const
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

		void Free()
		{
			if (m_ptr)
			{
				m_allocator.Free(m_ptr, sizeof(T));
				m_ptr = nullptr;
			}
		}

		// Warning : ptr must have been allocated with the same allocator
		void Assign(PtrType ptr) noexcept
		{
			Free();
			m_ptr = ptr;
		}

		[[nodiscard]] AllocatorRef<Allocator> GetAllocator() const noexcept { return m_allocator; }

		[[nodiscard]] const PtrType Get() const noexcept { return m_ptr; }
		[[nodiscard]] PtrType Get() noexcept { return m_ptr; }

		[[nodiscard]] const PtrType operator->() const noexcept { return m_ptr; }
		[[nodiscard]] PtrType operator->() noexcept { return m_ptr; }

		[[nodiscard]] const T& operator*() const noexcept { return *m_ptr; }
		[[nodiscard]] T& operator*() noexcept { return *m_ptr; }

		[[nodiscard]] explicit operator bool() const noexcept { return m_ptr != nullptr; }

	private:
		[[no_unique_address]] AllocatorRef<Allocator> m_allocator;
		PtrType m_ptr;
	};

	template<typename T, IAllocator Allocator, typename ...Args>
	UniquePtr<T, Allocator> AllocateUnique(AllocatorRef<Allocator> allocator, Args&&... args)
	{
		T* ptr = reinterpret_cast<T*>(allocator.Allocate(sizeof(T), alignof(T)));
		new (ptr) T(std::forward<Args>(args)...);
		return UniquePtr<T, Allocator>(ptr, allocator);
	}

	template<typename T, typename ...Args>
	UniquePtr<T, DefaultAllocator> MakeUnique(Args&&... args)
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

			SharedPtrControlBlock(U64 count, U64 weakCount, Function<void(void*, void*)> deallocate)
				: refCount(count), weakRefCount(weakCount), deallocate(std::move(deallocate))
			{}
		};
	}

	template<typename T>
	class SharedPtr;

	template<typename T>
	class WeakPtr
	{
	public:
		using ValueType = T;
		using PtrType = T*;

		WeakPtr()
			: m_ptr(nullptr)
			, m_controlBlock(nullptr)
		{}

		WeakPtr(const SharedPtr<T>& sharedPtr)
			: m_ptr(sharedPtr.m_ptr)
			, m_controlBlock(sharedPtr.m_controlBlock)
		{
			if (m_controlBlock)
				m_controlBlock->weakRefCount += 1;
		}

		WeakPtr(WeakPtr&& other) noexcept
			: m_ptr(other.m_ptr)
			, m_controlBlock(other.m_controlBlock)
		{
			other.m_ptr = nullptr;
			other.m_controlBlock = nullptr;
		}

		WeakPtr& operator=(WeakPtr&& other) noexcept
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

		WeakPtr& operator=(const WeakPtr& other) noexcept
		{
			WeakPtr copy(other);
			Swap(copy);
			return *this;
		}

		void Swap(WeakPtr& other) noexcept
		{
			std::swap(m_ptr, other.m_ptr);
			std::swap(m_controlBlock, other.m_controlBlock);
		}

		~WeakPtr()
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


		[[nodiscard]] SharedPtr<T> Lock() const;

		[[nodiscard]] explicit operator bool() const noexcept { return !IsEmpty(); }

		[[nodiscard]] bool IsEmpty() const noexcept { return m_ptr == nullptr || m_controlBlock->refCount == 0; }
	private:
		WeakPtr(Internal::SharedPtrControlBlock* controlBlock, PtrType ptr)
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

		SharedPtr(SharedPtr&& other) noexcept
			: m_controlBlock(other.m_controlBlock)
			, m_ptr(other.m_ptr)
		{
			other.m_controlBlock = nullptr;
			other.m_ptr = nullptr;
		}

		SharedPtr(const SharedPtr& other) noexcept
			: m_ptr(other.m_ptr)
			, m_controlBlock(other.m_controlBlock)
		{
			if (m_controlBlock)
				m_controlBlock->refCount += 1;
		}

		SharedPtr()
			: m_controlBlock(nullptr)
			, m_ptr(nullptr)
		{}

		~SharedPtr()
		{
			Release();
		}

		SharedPtr& operator=(SharedPtr&& other) noexcept
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

		SharedPtr& operator=(const SharedPtr& other) noexcept
		{
			SharedPtr copy(other);
			Swap(copy);
			return *this;
		}

		void Swap(SharedPtr& other)
		{
			std::swap(m_controlBlock, other.m_controlBlock);
			std::swap(m_ptr, other.m_ptr);
		}

		void Release()
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

		[[nodiscard]] bool IsEmpty() const { return m_ptr == nullptr; }

		// Warning : will make an extra allocation for the control block
		// Warning : ptr must have been allocated with [allocator]
		template<IAllocator Allocator>
		void Assign(PtrType ptr, AllocatorRef<Allocator> allocator = AllocatorRefDefaultArg<Allocator>()) noexcept
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

		[[nodiscard]] WeakPtr<T> GetWeak() const
		{
			return WeakPtr<T>(m_controlBlock, m_ptr);
		}

		[[nodiscard]] const PtrType Get() const noexcept { return m_ptr; }
		[[nodiscard]] PtrType Get() noexcept { return m_ptr; }

		[[nodiscard]] const PtrType operator->() const noexcept { return m_ptr; }
		[[nodiscard]] PtrType operator->() noexcept { return m_ptr; }

		[[nodiscard]] const T& operator*() const noexcept { return *m_ptr; }
		[[nodiscard]] T& operator*() noexcept { return *m_ptr; }

		[[nodiscard]] explicit operator bool() const noexcept { return m_ptr != nullptr; }

		[[nodiscard]] U64 NumRefs() const noexcept { return m_controlBlock ? m_controlBlock->refCount : 0; }
		[[nodiscard]] U64 NumWeakRefs() const noexcept { 
			if (!m_controlBlock)
				return 0;
			return m_controlBlock->refCount >= 1 ? m_controlBlock->weakRefCount - 1 : m_controlBlock->weakRefCount;
		}

	private:
		friend class WeakPtr<T>;

		template<typename T2, IAllocator Allocator, typename ...Args>
		friend SharedPtr<T2> MakeShared(AllocatorRef<Allocator> allocator, Args&& ...args);

		SharedPtr(PtrType ptr, Internal::SharedPtrControlBlock* controlBlock)
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
	[[nodiscard]] SharedPtr<T> WeakPtr<T>::Lock() const
	{
		return SharedPtr<T>(m_ptr, m_controlBlock);
	}

	// Warning : will make an extra allocation for the control block
	// Warning : ptr must have been allocated with [allocator]
	template<typename T, IAllocator Allocator>
	[[nodiscard]] SharedPtr<T> MakeSharedFromPtr(T* ptr, AllocatorRef<Allocator> allocator = AllocatorRefDefaultArg<Allocator>())
	{
		SharedPtr<T> sharedPtr;
		sharedPtr.Assign<Allocator>(ptr, allocator);
		return sharedPtr;
	}

	template<typename T, IAllocator Allocator, typename ...Args>
	[[nodiscard]] SharedPtr<T> MakeShared(AllocatorRef<Allocator> allocator, Args&& ...args)
	{
		struct DataAndControl
		{
			Internal::SharedPtrControlBlock controlBlock;
			T data;
		};

		DataAndControl* ptrDataAndControl = (DataAndControl*)allocator.Allocate(sizeof(DataAndControl), alignof(DataAndControl));

		// Yikes, can't control the type of the lambda capture so 
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
}
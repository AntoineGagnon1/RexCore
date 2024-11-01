#pragma once
#include <rexcore/core.hpp>
#include <rexcore/allocators.hpp>
#include <rexcore/concepts.hpp>

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
}
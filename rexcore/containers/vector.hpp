#pragma once

#include <rexcore/allocators.hpp>

namespace RexCore
{
	template<typename T, std::unsigned_integral IndexT, IAllocator Allocator>
	class VectorBase
	{
	public:
		using ValueType = T;
		using IndexType = IndexT;
		using AllocatorType = Allocator;

		constexpr VectorBase(const VectorBase&) = delete;
		constexpr VectorBase& operator=(const VectorBase&) = delete;

		constexpr VectorBase(VectorBase&&) noexcept = default;
		constexpr VectorBase& operator=(VectorBase&& other) noexcept = default;
		
		constexpr VectorBase() noexcept = default;
		constexpr ~VectorBase()
		{
			Free();
		}

		[[nodiscard]] constexpr T* Data() const
		{
			return m_data;
		}

		[[nodiscard]] constexpr IndexT Size() const
		{
			return m_size;
		}

		[[nodiscard]] constexpr IndexT Capacity() const
		{
			return m_capacity;
		}

		[[nodiscard]] constexpr bool IsEmpty() const
		{
			return m_size == 0;
		}

		[[nodiscard]] constexpr const T& First() const {
			REX_CORE_ASSERT(m_size > 0);
			return m_data[0];
		}

		[[nodiscard]] constexpr T& First() {
			REX_CORE_ASSERT(m_size > 0);
			return m_data[0];
		}

		[[nodiscard]] constexpr const T& Last() const {
			REX_CORE_ASSERT(m_size > 0);
			return m_data[m_size - 1];
		}

		[[nodiscard]] constexpr T& Last() {
			REX_CORE_ASSERT(m_size > 0);
			return m_data[m_size - 1];
		}

		[[nodiscard]] constexpr const T& operator[](IndexT index) const
		{
			REX_CORE_ASSERT(index < m_size);
			return m_data[index];
		}

		[[nodiscard]] constexpr T& operator[](IndexT index)
		{
			REX_CORE_ASSERT(index < m_size);
			return m_data[index];
		}

		constexpr void Reserve(IndexT newCapacity)
		{
			if (newCapacity <= m_capacity)
				return;

			// TODO perf : can we use realloc ?
			T* newData = static_cast<T*>(m_allocator.Allocate(newCapacity * sizeof(T), alignof(T)));
			if (m_data != nullptr)
			{
				for (IndexT i = 0; i < m_size; i++)
					newData[i] = std::move(m_data[i]);
				m_allocator.Free(m_data, m_capacity * sizeof(T));
			}
			m_data = newData;
			m_capacity = newCapacity;
		}

		template<typename ...Args>
		constexpr void Resize(IndexT newSize, Args&& ...constructorArgs)
		{
			if (newSize == 0)
			{
				Free();
			}
			else if (newSize < m_size)
			{
				T* newData = static_cast<T*>(m_allocator.Allocate(newSize * sizeof(T), alignof(T)));
				for (IndexT i = 0; i < newSize; i++)
					newData[i] = std::move(m_data[i]);

				for (IndexT i = newSize; i < m_size; i++)
					m_data[i].~T();

				m_allocator.Free(m_data, m_capacity * sizeof(T));
				m_data = newData;
				m_size = newSize;
				m_capacity = newSize;
			}
			else if (newSize > m_size)
			{
				Reserve(newSize);
				for (IndexT i = m_size; i < newSize; i++)
					new (&m_data[i]) T(std::forward<Args>(constructorArgs)...);
			}
		}

		constexpr void Clear()
		{
			for (IndexT i = 0; i < m_size; i++)
				m_data[i].~T();
			m_size = 0;
		}

		constexpr void Free()
		{
			Clear();
			if (m_data != nullptr)
			{
				m_allocator.Free(m_data, m_capacity * sizeof(T));
				m_data = nullptr;
				m_capacity = 0;
			}
		}

		constexpr T& PushBack(const T& value)
		{
			if (m_size == m_capacity)
				Reserve(m_capacity == 0 ? InitialSize : m_capacity * GrowthFactor);

			m_data[m_size] = value;
			return m_data[m_size++];
		}

		template<typename ...Args>
		constexpr T& EmplaceBack(Args&& ...constructorArgs)
		{
			if (m_size == m_capacity)
				Reserve(m_capacity == 0 ? InitialSize : m_capacity * GrowthFactor);

			new (&m_data[m_size]) T(std::forward<Args>(constructorArgs)...);
			return m_data[m_size++];
		}

		constexpr T& InsertAt(IndexT index, const T& value)
		{
			REX_CORE_ASSERT(index <= m_size);

			if (m_size == m_capacity) // TODO perf : could be faster if we resize and insert at the same time
				Reserve(m_capacity == 0 ? InitialSize : m_capacity * GrowthFactor);

			for (IndexT i = m_size; i > index; i--)
				m_data[i] = std::move(m_data[i - 1]);

			m_data[index] = value;
			m_size++;
			return m_data[index];
		}

		template<typename ...Args>
		constexpr T& EmplaceAt(IndexT index, Args&& ...constructorArgs)
		{
			REX_CORE_ASSERT(index <= m_size);

			if (m_size == m_capacity) // TODO perf : could be faster if we resize and insert at the same time
				Reserve(m_capacity == 0 ? InitialSize : m_capacity * GrowthFactor);

			for (IndexT i = m_size; i > index; i--)
				m_data[i] = std::move(m_data[i - 1]);

			new (&m_data[index]) T(std::forward<Args>(constructorArgs)...);
			m_size++;
			return m_data[index];
		}

		constexpr T PopBack()
		{
			REX_CORE_ASSERT(m_size > 0);

			m_size--;
			T value = std::move(m_data[m_size]);
			m_data[m_size].~T();
			return value;
		}

		// Swap with the last element and remove
		// WARNING : this will change the order of the elements
		constexpr void RemoveAt(IndexT index)
		{
			REX_CORE_ASSERT(index < m_size);

			m_data[index].~T();
			
			if (index != m_size - 1)
				m_data[index] = std::move(m_data[m_size - 1]);

			m_size--;
		}

		constexpr void RemoveAtOrdered(IndexT index)
		{
			REX_CORE_ASSERT(index < m_size);

			m_data[index].~T();

			for (IndexT i = index; i < m_size - 1; i++)
				m_data[i] = std::move(m_data[i + 1]);

			m_size--;
		}

	private:
		Allocator m_allocator;
		T* m_data = nullptr;
		IndexT m_size = 0;
		IndexT m_capacity = 0;

		static constexpr U64 GrowthFactor = 2;
		static constexpr U64 InitialSize = 8;
	};

	template<typename T, IAllocator Allocator = DefaultAllocator>
	using Vector = VectorBase<T, U32, Allocator>;

	template<typename T, IAllocator Allocator = DefaultAllocator>
	using SmallVector = VectorBase<T, U16, Allocator>;

	template<typename T, IAllocator Allocator = DefaultAllocator>
	using BigVector = VectorBase<T, U64, Allocator>;
}
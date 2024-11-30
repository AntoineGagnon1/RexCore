#pragma once

#include <rexcore/allocators.hpp>
#include <rexcore/core.hpp>
#include <rexcore/concepts.hpp>
#include <rexcore/containers/span.hpp>

#include <iterator>
#include <bit>

namespace RexCore
{
	// ParentClass must provide :
	// void Free()
	// T* Data()
	// IndexT Size()
	// IndexT Capacity()
	// void Reserve(IndexT newCapacity)
	// void SetSize(IndexT size)
	template<typename T, std::unsigned_integral IndexT, typename ParentClass, typename SpanT = SpanBase<T, IndexT>>
	class VectorTypeBase : public SpanTypeBase<T, IndexT, SpanT, ParentClass>
	{
	public:
		using ValueType = T;
		using IndexType = IndexT;
		using SpanType = SpanT;

		using Iterator = T*;
		using ConstIterator = const T*;
		static_assert(std::contiguous_iterator<Iterator>);
		static_assert(std::contiguous_iterator<ConstIterator>);

		REX_CORE_NO_COPY(VectorTypeBase);
		REX_CORE_DEFAULT_MOVE(VectorTypeBase);

		constexpr VectorTypeBase() noexcept = default;

		constexpr operator SpanType() const
		{
			auto self = static_cast<const ParentClass*>(this);
			return SpanType(self->Data(), self->Size());
		}

		constexpr ~VectorTypeBase()
		{
			static_cast<ParentClass*>(this)->Free();
		}

		[[nodiscard]] ParentClass Clone(this auto&& self)
		{
			static_assert(IClonable<T>, "The value type must be IClonable in order to clone a vector");
			ParentClass clone = [&] {
				if constexpr (requires { self.GetAllocator(); })
					return ParentClass(self.GetAllocator());
				else
					return ParentClass();
			}();

			clone.Reserve(self.Size());

			if constexpr (std::is_trivially_copyable_v<T>)
			{
				MemCopy(self.Data(), clone.Data(), self.Size() * sizeof(T));
				clone.SetSize(self.Size());
			}
			else
			{
				for (const T& item : self)
					clone.EmplaceBack(RexCore::Clone(item));
			}
			
			return clone;
		}

		[[nodiscard]] constexpr Iterator Begin()
		{
			auto self = static_cast<ParentClass*>(this);
			if (self->IsEmpty())
				return nullptr;
			return self->Data();
		}

		[[nodiscard]] constexpr ConstIterator Begin() const
		{
			auto self = static_cast<const ParentClass*>(this);
			if (self->IsEmpty())
				return nullptr;
			return self->Data();
		}

		[[nodiscard]] constexpr ConstIterator CBegin() const { return Begin(); }

		[[nodiscard]] constexpr Iterator End()
		{
			auto self = static_cast<ParentClass*>(this);
			if (self->IsEmpty())
				return nullptr;
			return self->Data() + self->Size();
		}

		[[nodiscard]] constexpr ConstIterator End() const
		{
			auto self = static_cast<const ParentClass*>(this);
			if (self->IsEmpty())
				return nullptr;
			return self->Data() + self->Size();
		}

		[[nodiscard]] constexpr ConstIterator CEnd() const { return End(); }

		constexpr void Clear(this auto&& self)
		{
			for (IndexT i = 0; i < self.Size(); i++)
				self.Data()[i].~T();
			self.SetSize(0);
		}

		constexpr T& PushBack(this auto&& self, const T& value)
		{
			static_assert(IClonable<T>, "The value type must be IClonable in order to PushBack into a vector");
			const IndexT size = self.Size();
			const IndexT capacity = self.Capacity();
			if (size == capacity)
				self.Reserve(capacity == 0 ? InitialSize : capacity * GrowthFactor);

			self.SetSize(size + 1);
			self.Data()[size] = RexCore::Clone(value);
			return self.Data()[size];
		}

		template<typename ...Args>
		constexpr T& EmplaceBack(this auto&& self, Args&& ...constructorArgs)
		{
			const IndexT size = self.Size();
			const IndexT capacity = self.Capacity();
			if (size == capacity)
				self.Reserve(capacity == 0 ? InitialSize : capacity * GrowthFactor);

			self.SetSize(size + 1);
			new (&self.Data()[size]) T(std::forward<Args>(constructorArgs)...);
			return self.Data()[size];
		}

		constexpr T& InsertAt(this auto&& self, IndexT index, const T& value)
		{
			static_assert(IClonable<T>, "The value type must be IClonable in order to InsertAt into a vector");
			const IndexT size = self.Size();
			const IndexT capacity = self.Capacity();
			REX_CORE_ASSERT(index <= size);

			if (size == capacity) // TODO perf : could be faster if we resize and insert at the same time
				self.Reserve(capacity == 0 ? InitialSize : capacity * GrowthFactor);

			for (IndexT i = size; i > index; i--)
				self.Data()[i] = std::move(self.Data()[i - 1]);

			self.Data()[index] = RexCore::Clone(value);
			self.SetSize(size + 1);
			return self.Data()[index];
		}

		template<typename ...Args>
		constexpr T& EmplaceAt(this auto&& self, IndexT index, Args&& ...constructorArgs)
		{
			const IndexT size = self.Size();
			const IndexT capacity = self.Capacity();
			REX_CORE_ASSERT(index <= size);

			if (size == capacity) // TODO perf : could be faster if we resize and insert at the same time
				self.Reserve(capacity == 0 ? InitialSize : capacity * GrowthFactor);

			for (IndexT i = size; i > index; i--)
				self.Data()[i] = std::move(self.Data()[i - 1]);

			new (&self.Data()[index]) T(std::forward<Args>(constructorArgs)...);
			self.SetSize(size + 1);
			return self.Data()[index];
		}

		constexpr T PopBack(this auto&& self)
		{
			const IndexT size = self.Size();
			REX_CORE_ASSERT(size > 0);

			T value = std::move(self.Data()[size - 1]);
			self.Data()[size - 1].~T(); // TODO : probably not necessary, moved object should do nothing in the destructor
			self.SetSize(size - 1);
			return value;
		}

		// Swap with the last element and remove
		// WARNING : this will change the order of the elements
		constexpr void RemoveAt(this auto&& self, IndexT index)
		{
			const IndexT size = self.Size();
			REX_CORE_ASSERT(index < size);

			self.Data()[index].~T();

			if (index != size - 1)
				self.Data()[index] = std::move(self.Data()[size - 1]);

			self.SetSize(size - 1);
		}

		// Swap with the last element and remove
		// WARNING : this will change the order of the elements
		constexpr void Remove(this auto&& self, const T& value)
		{
			self.RemoveAt(self.IndexOf(value));
		}

		constexpr void RemoveAtOrdered(this auto&& self, IndexT index)
		{
			const IndexT size = self.Size();
			REX_CORE_ASSERT(index < size);

			self.Data()[index].~T();

			for (IndexT i = index; i < size - 1; i++)
				self.Data()[i] = std::move(self.Data()[i + 1]);

			self.SetSize(size - 1);
		}

		constexpr void RemoveOrdered(this auto&& self, const T& value)
		{
			self.RemoveAtOrdered(self.IndexOf(value));
		}

	public:
		// For ranged-based for and other STL functions
		[[nodiscard]] constexpr Iterator begin() { return Begin(); }
		[[nodiscard]] constexpr ConstIterator begin() const { return Begin(); }
		[[nodiscard]] constexpr Iterator end() { return End(); }
		[[nodiscard]] constexpr ConstIterator end() const { return End(); }
		[[nodiscard]] constexpr ConstIterator cbegin() const { return CBegin(); }
		[[nodiscard]] constexpr ConstIterator cend() const { return CEnd(); }

	private:
		static constexpr U64 GrowthFactor = 2;
		static constexpr U64 InitialSize = 8;
	};

	template<typename T, std::unsigned_integral IndexT, IAllocator Allocator>
	class VectorBase : public VectorTypeBase<T, IndexT, VectorBase<T, IndexT, Allocator>>
	{
	private:
		using BaseT = VectorTypeBase<T, IndexT, VectorBase<T, IndexT, Allocator>>;

	public:
		using AllocatorType = Allocator;

		REX_CORE_NO_COPY(VectorBase);
		REX_CORE_DEFAULT_MOVE(VectorBase);
		
		constexpr VectorBase(AllocatorRef<Allocator> allocator = AllocatorRefDefaultArg<Allocator>()) noexcept
			: m_allocator(allocator)
		{}
		
		explicit constexpr VectorBase(BaseT::SpanType from, AllocatorRef<Allocator> allocator = AllocatorRefDefaultArg<Allocator>())
			: m_allocator(allocator)
		{
			Reserve(from.Size());
			for (const T& item : from)
				BaseT::PushBack(item);
		}

		[[nodiscard]] constexpr const T* Data() const { return m_data; }
		[[nodiscard]] constexpr T* Data() { return m_data; }
		[[nodiscard]] constexpr IndexT Size() const { return m_size; }
		[[nodiscard]] constexpr IndexT Capacity() const { return m_capacity; }
		[[nodiscard]] constexpr AllocatorRef<Allocator> GetAllocator() const { return m_allocator; }

		constexpr void Reserve(IndexT newCapacity)
		{
			if (newCapacity <= m_capacity)
				return;

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

				m_size = newSize;
			}
		}

		constexpr void Free()
		{
			Base::Clear();
			if (m_data != nullptr)
			{
				m_allocator.Free(m_data, m_capacity * sizeof(T));
				m_data = nullptr;
				m_capacity = 0;
			}
		}

	private:
		constexpr void SetSize(IndexT size)
		{
			m_size = size;
		}

	private:
		[[no_unique_address]] AllocatorRef<Allocator> m_allocator;
		T* m_data = nullptr;
		IndexT m_size = 0;
		IndexT m_capacity = 0;

		using Base = VectorTypeBase<T, IndexT, VectorBase<T, IndexT, Allocator>>;
		friend class Base;
	};

	template<typename T, std::unsigned_integral IndexT, IndexT InplaceSize, IAllocator Allocator>
	class InplaceVectorBase : public VectorTypeBase<T, IndexT, InplaceVectorBase<T, IndexT, InplaceSize, Allocator>>
	{
	private:
		using BaseT = VectorTypeBase<T, IndexT, InplaceVectorBase<T, IndexT, InplaceSize, Allocator>>;

	public:
		using AllocatorType = Allocator;
		constexpr static IndexT InplaceCapacity = InplaceSize;

		REX_CORE_NO_COPY(InplaceVectorBase);
		REX_CORE_DEFAULT_MOVE(InplaceVectorBase);

		constexpr InplaceVectorBase(AllocatorRef<Allocator> allocator = AllocatorRefDefaultArg<Allocator>()) noexcept
			: m_allocator(allocator)
		{}

		explicit constexpr InplaceVectorBase(BaseT::SpanType from, AllocatorRef<Allocator> allocator = AllocatorRefDefaultArg<Allocator>())
			: m_allocator(allocator)
		{
			Reserve(from.Size());
			for (const T& item : from)
				BaseT::PushBack(item);
		}

		[[nodiscard]] constexpr const T* Data() const { return m_data; }
		[[nodiscard]] constexpr T* Data() { return m_data; }
		[[nodiscard]] constexpr IndexT Size() const { return m_size; }
		[[nodiscard]] constexpr IndexT Capacity() const { return m_capacity; }
		[[nodiscard]] constexpr AllocatorRef<Allocator> GetAllocator() const { return m_allocator; }

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

				if (m_data != m_inplaceData)
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
				if (m_data == m_inplaceData)
				{
					for (IndexT i = newSize; i < m_size; i++)
						m_data[i].~T();

					m_size = newSize;
				}
				else
				{
					T* newData = newSize <= InplaceSize ? m_inplaceData : static_cast<T*>(m_allocator.Allocate(newSize * sizeof(T), alignof(T)));
					for (IndexT i = 0; i < newSize; i++)
						newData[i] = std::move(m_data[i]);

					for (IndexT i = newSize; i < m_size; i++)
						m_data[i].~T();

					m_allocator.Free(m_data, m_capacity * sizeof(T));
					m_data = newData;
					m_size = newSize;
					m_capacity = newSize <= InplaceSize ? InplaceSize : newSize;
				}
			}
			else if (newSize > m_size)
			{
				Reserve(newSize);
				for (IndexT i = m_size; i < newSize; i++)
					new (&m_data[i]) T(std::forward<Args>(constructorArgs)...);

				m_size = newSize;
			}
		}

		constexpr void Free()
		{
			Base::Clear();
			if (m_data != nullptr && m_data != m_inplaceData)
			{
				m_allocator.Free(m_data, m_capacity * sizeof(T));
				m_data = m_inplaceData;
				m_capacity = InplaceSize;
			}
		}

	private:
		constexpr void SetSize(IndexT size)
		{
			m_size = size;
		}

	private:
		[[no_unique_address]] AllocatorRef<Allocator> m_allocator;
		T* m_data = m_inplaceData;
		IndexT m_size = 0;
		IndexT m_capacity = InplaceSize;

		union { // union to prevent default initialization of array elements
			T m_inplaceData[InplaceSize];
		};
		using Base = VectorTypeBase<T, IndexT, InplaceVectorBase<T, IndexT, InplaceSize, Allocator>>;
		friend class Base;
	};

	// This vector type will not grow beyond MaxSize
	template<typename T, std::unsigned_integral IndexT, IndexT MaxSize>
	class FixedVectorBase : public VectorTypeBase<T, IndexT, FixedVectorBase<T, IndexT, MaxSize>>
	{
	private:
		using BaseT = VectorTypeBase<T, IndexT, FixedVectorBase<T, IndexT, MaxSize>>;

	public:
		constexpr static IndexT FixedSize = MaxSize;

		REX_CORE_NO_COPY(FixedVectorBase);
		REX_CORE_DEFAULT_MOVE(FixedVectorBase);

		constexpr FixedVectorBase() noexcept
			: m_size(0) {}

		explicit constexpr FixedVectorBase(BaseT::SpanType from)
		{
			Reserve(from.Size());
			for (const T& item : from)
				BaseT::PushBack(item);
		}

		[[nodiscard]] constexpr const T* Data() const { return static_cast<const T*>(static_cast<const void*>(m_inplaceData)); }
		[[nodiscard]] constexpr T* Data() { return static_cast<T*>(static_cast<void*>(m_inplaceData)); }
		[[nodiscard]] constexpr IndexT Size() const { return m_size; }
		[[nodiscard]] constexpr IndexT Capacity() const { return MaxSize; }

		constexpr void Reserve(IndexT newCapacity)
		{
			REX_CORE_ASSERT(newCapacity <= MaxSize);
		}

		template<typename ...Args>
		constexpr void Resize(IndexT newSize, Args&& ...constructorArgs)
		{
			REX_CORE_ASSERT(newSize <= MaxSize);

			if (newSize == 0)
			{
				Free();
			}
			else if (newSize < m_size)
			{
				for (IndexT i = newSize; i < m_size; i++)
					Data()[i].~T();

				m_size = newSize;
			}
			else if (newSize > m_size)
			{
				for (IndexT i = m_size; i < newSize; i++)
					new (&Data()[i]) T(std::forward<Args>(constructorArgs)...);

				m_size = newSize;
			}
		}

		constexpr void Free()
		{
			Base::Clear();
		}

	private:
		constexpr void SetSize(IndexT size)
		{
			REX_CORE_ASSERT(size <= MaxSize);
			m_size = size;
		}

	private:
		union {
			T m_inplaceData[MaxSize];
		};
		IndexT m_size = 0;

		using Base = VectorTypeBase<T, IndexT, FixedVectorBase<T, IndexT, MaxSize>>;
		friend class Base;
	};

	template<typename T, IAllocator Allocator = DefaultAllocator>
	using SmallVector = VectorBase<T, U16, Allocator>;

	template<typename T, IAllocator Allocator = DefaultAllocator>
	using Vector = VectorBase<T, U32, Allocator>;

	template<typename T, IAllocator Allocator = DefaultAllocator>
	using BigVector = VectorBase<T, U64, Allocator>;


	template<typename T, U16 InplaceSize, IAllocator Allocator = DefaultAllocator>
	using SmallInplaceVector = InplaceVectorBase<T, U16, InplaceSize, Allocator>;

	template<typename T, U32 InplaceSize, IAllocator Allocator = DefaultAllocator>
	using InplaceVector = InplaceVectorBase<T, U32, InplaceSize, Allocator>;

	template<typename T, U64 InplaceSize, IAllocator Allocator = DefaultAllocator>
	using BigInplaceVector = InplaceVectorBase<T, U64, InplaceSize, Allocator>;


	template<typename T, U16 MaxSize>
	using SmallFixedVector = FixedVectorBase<T, U16, MaxSize>;

	template<typename T, U32 MaxSize>
	using FixedVector = FixedVectorBase<T, U32, MaxSize>;

	template<typename T, U64 MaxSize>
	using BigFixedVector = FixedVectorBase<T, U64, MaxSize>;
}
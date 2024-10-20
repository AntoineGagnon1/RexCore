#pragma once
#include <rexcore/core.hpp>

#include <concepts>

namespace RexCore
{
	template<typename T, std::unsigned_integral IndexT, typename SpanT, typename ParentT>
	class SpanTypeBase
	{
	public:
		using ValueType = T;
		using IndexType = IndexT;

		using Iterator = T*;
		using ConstIterator = const T*;

		static_assert(std::contiguous_iterator<Iterator>);
		static_assert(std::contiguous_iterator<ConstIterator>);

		[[nodiscard]] constexpr decltype(auto) operator[](this auto&& self, IndexT index)
		{
			REX_CORE_ASSERT(index < self.Size());
			return self.Data()[index];
		}

		[[nodiscard]] constexpr bool IsEmpty(this auto&& self)
		{
			return self.Size() == 0;
		}

		[[nodiscard]] constexpr decltype(auto) First(this auto&& self)
		{
			REX_CORE_ASSERT(self.Size() > 0);
			return self.Data()[0];
		}

		[[nodiscard]] constexpr decltype(auto) Last(this auto&& self)
		{
			REX_CORE_ASSERT(self.Size() > 0);
			return self.Data()[self.Size() - 1];
		}

		[[nodiscard]] constexpr Iterator Begin()
		{
			auto self = static_cast<ParentT*>(this);
			if (self->IsEmpty())
				return nullptr;
			return self->Data();
		}

		[[nodiscard]] constexpr ConstIterator Begin() const
		{
			auto self = static_cast<const ParentT*>(this);
			if (self->IsEmpty())
				return nullptr;
			return self->Data();
		}

		[[nodiscard]] constexpr ConstIterator CBegin() const { return Begin(); }

		[[nodiscard]] constexpr Iterator End()
		{
			auto self = static_cast<ParentT*>(this);
			if (self->IsEmpty())
				return nullptr;
			return self->Data() + self->Size();
		}

		[[nodiscard]] constexpr ConstIterator End() const
		{
			auto self = static_cast<const ParentT*>(this);
			if (self->IsEmpty())
				return nullptr;
			return self->Data() + self->Size();
		}

		[[nodiscard]] constexpr ConstIterator CEnd() const { return End(); }

		[[nodiscard]] constexpr bool Contains(this auto&& self, const T& value)
		{
			for (const auto& item : self)
			{
				if (item == value)
					return true;
			}
			return false;
		}

		[[nodiscard]] constexpr SpanT SubSpan(this auto&& self, IndexT start, IndexT length)
		{
			if (start >= self.Size())
				return SpanT();

			return SpanT(self.Data() + start, Min(length, self.Size() - start));
		}

	public:
		// For ranged-based for and other STL functions
		[[nodiscard]] constexpr Iterator begin() { return Begin(); }
		[[nodiscard]] constexpr ConstIterator begin() const { return Begin(); }
		[[nodiscard]] constexpr Iterator end() { return End(); }
		[[nodiscard]] constexpr ConstIterator end() const { return End(); }
		[[nodiscard]] constexpr ConstIterator cbegin() const { return CBegin(); }
		[[nodiscard]] constexpr ConstIterator cend() const { return CEnd(); }
	};

	template<typename T, std::unsigned_integral IndexT>
	class SpanBase : public SpanTypeBase<T, IndexT, SpanBase<T, IndexT>, SpanBase<T, IndexT>>
	{
	public:
		REX_CORE_DEFAULT_COPY(SpanBase);
		REX_CORE_DEFAULT_MOVE(SpanBase);

		constexpr SpanBase()
			: m_data(nullptr), m_size(0)
		{}

		constexpr SpanBase(const T* data, IndexT size)
			: m_data(data), m_size(size)
		{}

		[[nodiscard]] constexpr IndexT Size() const { return m_size; }
		[[nodiscard]] constexpr const T* Data() const { return m_data; }

		[[nodiscard]] constexpr const T& operator[](IndexT index) const
		{
			REX_CORE_ASSERT(index < m_size);
			return m_data[index];
		}

	private:
		const T* m_data;
		IndexT m_size;
	};

	template<typename T>
	using Span = SpanBase<T, U32>;
	template<typename T>
	using SmallSpan = SpanBase<T, U16>;
	template<typename T>
	using BigSpan = SpanBase<T, U64>;
}
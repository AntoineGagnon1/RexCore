#pragma once
#include <rexcore/core.hpp>
#include <rexcore/math.hpp>

#include <concepts>

namespace RexCore
{
	template<typename T, std::unsigned_integral IndexT, typename SpanT, typename ParentT>
	class SpanTypeBase
	{
	public:
		using ValueType = T;
		using IndexType = IndexT;

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

		[[nodiscard]] constexpr bool Contains(this auto&& self, const T& value)
		{
			for (const T& found : self)
			{
				if (found == value)
					return true;
			}
			return false;
		}

		[[nodiscard]] constexpr SpanT SubSpan(this auto&& self, IndexT start, IndexT length = Math::MaxValue<IndexT>())
		{
			if (start >= self.Size())
				return SpanT();

			return SpanT(self.Data() + start, Math::Min<IndexT>(length, self.Size() - start));
		}
	};

	template<typename T, std::unsigned_integral IndexT>
	class SpanBase : public SpanTypeBase<T, IndexT, SpanBase<T, IndexT>, SpanBase<T, IndexT>>
	{
	public:
		using ConstIterator = const T*;
		static_assert(std::contiguous_iterator<ConstIterator>);


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

		[[nodiscard]] constexpr ConstIterator Begin() const { return m_data; }
		[[nodiscard]] constexpr ConstIterator CBegin() const { return m_data; }

		[[nodiscard]] constexpr ConstIterator End() const { return m_data + m_size; }
		[[nodiscard]] constexpr ConstIterator CEnd() const { return m_data + m_size; }

	public:
		// For ranged-based for and other STL functions
		[[nodiscard]] constexpr ConstIterator begin() const { return Begin(); }
		[[nodiscard]] constexpr ConstIterator end() const { return End(); }
		[[nodiscard]] constexpr ConstIterator cbegin() const { return CBegin(); }
		[[nodiscard]] constexpr ConstIterator cend() const { return CEnd(); }

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
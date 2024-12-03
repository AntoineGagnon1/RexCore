#pragma once

#include <tuple>

namespace RexCore::Iter
{
	template<typename T>
	using GetValueType = typename T::ValueType;

	template<typename T>
	using GetIndexType = typename T::IndexType;

	template<typename T>
	using GetIteratorType = typename T::Iterator;

	namespace Details {
		template <typename ... Args, std::size_t ... Index>
		inline bool MatchAnyImpl(std::tuple<Args...> const& lhs, std::tuple<Args...> const& rhs, std::index_sequence<Index...>)
		{
			auto result = false;
			result = (... | (std::get<Index>(lhs) == std::get<Index>(rhs)));
			return result;
		}

		template <typename ... Args>
		inline bool MatchAny(std::tuple<Args...> const& lhs, std::tuple<Args...> const& rhs)
		{
			return MatchAnyImpl(lhs, rhs, std::index_sequence_for<Args...>{});
		}

		template<typename T, typename... U>
		struct BiggestTypeImpl {
			using Tm = typename BiggestTypeImpl<U...>::Type;
			using Type = std::conditional_t<sizeof(T) < sizeof(Tm), Tm, T>;
		};

		template<typename T>
		struct BiggestTypeImpl<T> {
			using Type = T;
		};

		template<typename ...T>
		using BiggestType = typename BiggestTypeImpl<T...>::Type;
	}

	template<typename T>
	[[nodiscard]] constexpr decltype(auto) Begin(T& container)
	{
		return container.Begin();
	}

	template<typename T>
	[[nodiscard]] constexpr decltype(auto) Begin(const T& container)
	{
		return container.Begin();
	}

	template<typename T>
	[[nodiscard]] constexpr decltype(auto) End(T& container)
	{
		return container.End();
	}

	template<typename T>
	[[nodiscard]] constexpr decltype(auto) End(const T& container)
	{
		return container.End();
	}

	template<typename ...Types>
	struct ZipIterator
	{
	public:
		using ValueType = std::tuple<std::add_lvalue_reference_t<GetValueType<Types>>...>;

		// For std
		using value_type = ValueType;

		ZipIterator(std::tuple<GetIteratorType<Types>...> iterators) noexcept
			: m_iterators(iterators)
		{}

		[[nodiscard]] friend bool operator==(const ZipIterator& lhs, const ZipIterator& rhs)
		{
			// or instead of and to make the for each loops stop when one of the iterators is at the end
			return Details::MatchAny(lhs.m_iterators, rhs.m_iterators);
		}
		[[nodiscard]] friend bool operator!=(const ZipIterator& lhs, const ZipIterator& rhs)
		{
			return !(lhs == rhs);
		}

		ZipIterator& operator++()
		{
			std::apply([](auto& ...iter) { ((++iter), ...); }, m_iterators);
			return *this;
		}
		ZipIterator operator++(int)
		{
			ZipIterator copy(*this);
			++*this;
			return copy;
		}

		[[nodiscard]] ValueType operator*()
		{
			return std::apply([](auto& ...iter) { return ValueType(*iter...); }, m_iterators);
		}

	private:
		std::tuple<GetIteratorType<Types>...> m_iterators;
	};

	template<typename ...Types>
	class Zip
	{
	public:
		using Iterator = ZipIterator<Types...>;

		Zip(Types& ...containers) noexcept
			: m_containers(containers...)
		{}

		[[nodiscard]] Iterator Begin() noexcept
		{
			return Iterator(std::apply([](auto& ...container) { return std::make_tuple(Iter::Begin(container)...); }, m_containers));
		}
		[[nodiscard]] Iterator End() noexcept
		{
			return Iterator(std::apply([](auto& ...container) { return std::make_tuple(Iter::End(container)...); }, m_containers));
		}

		[[nodiscard]] Iterator begin() noexcept { return Begin(); }
		[[nodiscard]] Iterator end() noexcept { return End(); }

	private:
		std::tuple<Types&...> m_containers;
	};

	template<typename ValueT>
	class IntegerIterator
	{
	public:
		using ValueType = ValueT;

		// For std
		using value_type = ValueType;

		IntegerIterator(ValueT value)
			: m_value(value)
		{}

		[[nodiscard]] friend bool operator==(const IntegerIterator& lhs, const IntegerIterator& rhs) { return lhs.m_value == rhs.m_value; }
		[[nodiscard]] friend bool operator!=(const IntegerIterator& lhs, const IntegerIterator& rhs) { return !(lhs == rhs); }

		IntegerIterator& operator++()
		{
			++m_value;
			return *this;
		}
		IntegerIterator operator++(int)
		{
			IntegerIterator copy(*this);
			++*this;
			return copy;
		}

		[[nodiscard]] const ValueType& operator*() const
		{
			return m_value;
		}

		[[nodiscard]] ValueType& operator*()
		{
			return m_value;
		}


	private:
		ValueT m_value;
	};

	template<typename ValueT>
	class IntegerRange
	{
	public:
		using Iterator = IntegerIterator<ValueT>;
		using ValueType = ValueT;

		IntegerRange() : IntegerRange(ValueT(0), ValueT(0)) {}

		IntegerRange(ValueT start, ValueT end)
			: m_begin(start), m_end(end)
		{}

		[[nodiscard]] Iterator Begin() noexcept { return Iterator(m_begin); }
		[[nodiscard]] Iterator End() noexcept { return Iterator(m_end); }
		[[nodiscard]] Iterator begin() noexcept { return Begin(); }
		[[nodiscard]] Iterator end() noexcept { return End(); }

	private:
		ValueT m_begin;
		ValueT m_end;
	};

	template<typename ...ArrayT>
	class Enumerate
	{
		using ZipType = Zip<IntegerRange<Details::BiggestType<GetIndexType<ArrayT>...>>, ArrayT...>;
		using IndexType = Details::BiggestType<GetIndexType<ArrayT>...>;
		using Iterator = typename ZipType::Iterator;
	public:
		Enumerate(ArrayT& ...arrays)
			: m_integerRange(static_cast<IndexType>(0), MinSize(arrays...)), m_zip(m_integerRange, arrays...)
		{}

		[[nodiscard]] Iterator Begin() noexcept { return m_zip.Begin(); }
		[[nodiscard]] Iterator End() noexcept { return m_zip.End(); }
		[[nodiscard]] Iterator begin() noexcept { return Begin(); }
		[[nodiscard]] Iterator end() noexcept { return End(); }

	private:
		IndexType MinSize(const ArrayT& ...arrays) const
		{
			IndexType minSize = Math::MaxValue<IndexType>();
			([&](const ArrayT& arr) { minSize = Math::Min(minSize, arr.Size()); }(arrays), ...);
			return minSize;
		}

		IntegerRange<IndexType> m_integerRange;
		ZipType m_zip;
	};
}
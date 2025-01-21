#pragma once

#include <tuple>

namespace RexCore::Iter
{
	namespace Details {
		template <typename ... Args, std::size_t ... Index>
		inline constexpr bool MatchAnyImpl(std::tuple<Args...> const& lhs, std::tuple<Args...> const& rhs, std::index_sequence<Index...>)
		{
			auto result = false;
			result = (... | (std::get<Index>(lhs) == std::get<Index>(rhs)));
			return result;
		}

		template <typename ... Args>
		inline constexpr bool MatchAny(std::tuple<Args...> const& lhs, std::tuple<Args...> const& rhs)
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
	
	// A view is a copyable, iterable object with an Iterator subtypes
	template<typename T>
	concept IView = std::is_copy_constructible_v<T> && std::is_copy_assignable_v<T> && requires {
		typename T::Iterator;
		{ std::declval<T>().Begin() } -> std::convertible_to<typename T::Iterator>;
		{ std::declval<T>().End() } -> std::convertible_to<typename T::Iterator>;
	};

	// An iterator is a copyable object with operator*, operator++, operator== and operator!=
	template<typename T>
	concept IIterator = std::is_copy_constructible_v<T> && std::is_copy_assignable_v<T> && requires (T& t, T& t2) {
		{ *t };
		{ t++ } -> std::convertible_to<T>;
		{ ++t } -> std::convertible_to<T>;
		{ t == t2 } -> std::convertible_to<bool>;
		{ t != t2 } -> std::convertible_to<bool>;
	};

	template<typename T>
	using ViewGetIteratorType = decltype(std::declval<std::add_const_t<T>>().Begin());

	template<IIterator It>
	using IteratorGetValueType = decltype(*std::declval<It>()); // Result of calling operator* on the iterator type

	template<IIterator IteratorT>
	class ContainerView
	{
	public:
		using Iterator = IteratorT;

		constexpr ContainerView(IteratorT begin, IteratorT end)
			: m_begin(begin), m_end(end)
		{
			static_assert(IView<ContainerView<IteratorT>>);
		}

		[[nodiscard]] constexpr Iterator Begin() const noexcept { return m_begin; }
		[[nodiscard]] constexpr Iterator End() const noexcept { return m_end; }
		[[nodiscard]] constexpr Iterator begin() const noexcept { return m_begin; }
		[[nodiscard]] constexpr Iterator end() const noexcept { return m_end; }
	private:
		IteratorT m_begin, m_end;
	};

	template<IIterator ...Iterators>
	struct ZipIterator
	{
	public:
		using ValueType = std::tuple<IteratorGetValueType<Iterators>...>;

		// For std
		using value_type = ValueType;

		constexpr ZipIterator(std::tuple<Iterators...> iterators) noexcept
			: m_iterators(iterators)
		{
			static_assert(IIterator<ZipIterator<Iterators...>>);
		}

		[[nodiscard]] friend constexpr bool operator==(const ZipIterator& lhs, const ZipIterator& rhs)
		{
			// || instead of && to make foreach loops stop when one of the iterators is at the end
			return Details::MatchAny(lhs.m_iterators, rhs.m_iterators);
		}
		[[nodiscard]] friend constexpr bool operator!=(const ZipIterator& lhs, const ZipIterator& rhs)
		{
			return !(lhs == rhs);
		}

		constexpr ZipIterator& operator++()
		{
			std::apply([](auto& ...iter) { ((++iter), ...); }, m_iterators);
			return *this;
		}
		constexpr ZipIterator operator++(int)
		{
			ZipIterator copy(*this);
			++*this;
			return copy;
		}

		[[nodiscard]] constexpr ValueType operator*()
		{
			return std::apply([](auto& ...iter) { return ValueType(*iter...); }, m_iterators);
		}

	private:
		std::tuple<Iterators...> m_iterators;
	};

	template<IView ...Views>
	class Zip
	{
	public:
		using Iterator = ZipIterator<ViewGetIteratorType<Views>...>;

		Zip(Views ...views) noexcept
			: m_views(views...)
		{
			static_assert(IView<Zip<Views...>>);
		}

		[[nodiscard]] Iterator Begin() const noexcept
		{
			return Iterator(std::apply([](auto& ...view) { return std::make_tuple(Iter::Begin(view)...); }, m_views));
		}
		[[nodiscard]] Iterator End() const noexcept
		{
			return Iterator(std::apply([](auto& ...view) { return std::make_tuple(Iter::End(view)...); }, m_views));
		}

		[[nodiscard]] Iterator begin() const noexcept { return Begin(); }
		[[nodiscard]] Iterator end() const noexcept { return End(); }

	private:
		std::tuple<Views...> m_views;
	};

	// Deduction guide so calls to Zip with containers can be deduced to Views
	template <typename... Containers>
	Zip(Containers...) -> Zip<std::conditional_t<IView<Containers>, std::remove_reference_t<Containers>, ContainerView<ViewGetIteratorType<Containers>>>...>;

	template<std::integral ValueT>
	class IntegerIterator
	{
	public:
		// For std
		using value_type = ValueT;

		IntegerIterator(ValueT value)
			: m_value(value)
		{
			static_assert(IIterator<IntegerIterator<ValueT>>);
		}

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

		[[nodiscard]] const ValueT& operator*() const { return m_value; }
		[[nodiscard]] ValueT& operator*() { return m_value; }


	private:
		ValueT m_value;
	};

	template<std::integral ValueT>
	class IntegerRange
	{
	public:
		using Iterator = IntegerIterator<ValueT>;

		IntegerRange(ValueT startInclusive, ValueT endExclusive)
			: m_begin(startInclusive), m_end(endExclusive)
		{
			static_assert(IView<IntegerRange<ValueT>>);
		}

		[[nodiscard]] Iterator Begin() const noexcept { return m_begin; }
		[[nodiscard]] Iterator End() const noexcept { return m_end; }
		[[nodiscard]] Iterator begin() const noexcept { return Begin(); }
		[[nodiscard]] Iterator end() const noexcept { return End(); }

	private:
		Iterator m_begin;
		Iterator m_end;
	};

	template<IView ...Views>
	class Enumerate
	{
		using ZipType = Zip<IntegerRange<U64>, Views...>;
		using IndexType = U64;
	public:
		using Iterator = typename ZipType::Iterator;
		
		Enumerate(Views ...arrays)
			: m_integerRange(static_cast<IndexType>(0), Math::MaxValue<IndexType>()), m_zip(m_integerRange, arrays...)
		{
			static_assert(sizeof ...(Views) >= 1, "You must pass at least one view to RexCore::Enumerate");
			static_assert(IView<Enumerate<Views...>>);
		}

		[[nodiscard]] Iterator Begin() const noexcept { return m_zip.Begin(); }
		[[nodiscard]] Iterator End() const noexcept { return m_zip.End(); }
		[[nodiscard]] Iterator begin() const noexcept { return Begin(); }
		[[nodiscard]] Iterator end() const noexcept { return End(); }

	private:
		IntegerRange<IndexType> m_integerRange;
		ZipType m_zip;
	};

	template <typename... Containers>
	Enumerate(Containers...) -> Enumerate<std::conditional_t<IView<Containers>, std::remove_reference_t<Containers>, ContainerView<ViewGetIteratorType<Containers>>>...>;

	// Safe with views that contain less than toSkip elements
	template<IView View>
	class Skip
	{
	public:
		using Iterator = ViewGetIteratorType<View>;

		Skip(U64 toSkip, View view)
			: m_view(view), m_begin(view.Begin())
		{
			static_assert(IView<Skip<View>>);

			auto end = m_view.End();
			for (U64 i = 0; i < toSkip && m_begin != end; i++)
				++m_begin;
		}

		[[nodiscard]] Iterator Begin() const noexcept {  return m_begin; }
		[[nodiscard]] Iterator End() const noexcept { return m_view.End(); }
		[[nodiscard]] Iterator begin() const noexcept { return m_begin; }
		[[nodiscard]] Iterator end() const noexcept { return End(); }

	private:
		View m_view;
		Iterator m_begin;
	};

	template<typename Container>
	Skip(U64, Container) -> Skip<std::conditional_t<IView<Container>, std::remove_reference_t<Container>, ContainerView<ViewGetIteratorType<Container>>>>;

}
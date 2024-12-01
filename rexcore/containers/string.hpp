#pragma once

#include <rexcore/containers/vector.hpp>
#include <rexcore/containers/span.hpp>
#include <rexcore/allocators.hpp>

#include <rexcore/vendors/unordered_dense.hpp> // for ankerl::unordered_dense::hash

#include <cstring>
#include <compare>
#include <string_view>
#include <ostream>

namespace RexCore
{
	inline U64 StringLength(const char* str)
	{
		return std::strlen(str);
	}

	inline U64 StringLength(const wchar_t* wstr)
	{
		return std::wcslen(wstr);
	}

	inline S32 StringCompare(const char* a, const char* b)
	{
		return std::strcmp(a, b);
	}

	inline S32 StringCompare(const wchar_t* a, const wchar_t* b)
	{
		return std::wcscmp(a, b);
	}

	inline S32 StringCompare(const char* a, const char* b, U64 length)
	{
		return std::strncmp(a, b, length);
	}

	inline S32 StringCompare(const wchar_t* a, const wchar_t* b, U64 length)
	{
		return std::wcsncmp(a, b, length);
	}

	namespace Internal
	{
		inline U64 CalcGrowSize(U64 currentSize, U64 newSize)
		{
			return Math::Max(Math::NextPowerOfTwo(currentSize), newSize);
		}
	}

	// Base for string-like types, see String and StringView
	template<typename CharT, typename StringViewT, typename ParentT>
	class StringTypeBase
	{
	public:
		using CharType = CharT;
		
		[[nodiscard]] constexpr StringViewT SubStr(this auto&& self, U64 start, U64 length = Math::MaxValue<U64>())
		{
			if (start >= self.Size())
				return StringViewT();

			return StringViewT(self.Data() + start, Math::Min<U64>(length, self.Size() - start));
		}

		[[nodiscard]] constexpr bool StartsWith(this auto&& self, StringViewT startsWith)
		{
			return self.Size() >= startsWith.Size() && self.SubStr(0, startsWith.Size()) == startsWith;
		}

		[[nodiscard]] constexpr bool EndsWith(this auto&& self, StringViewT endsWith)
		{
			return self.Size() >= endsWith.Size() && self.SubStr(self.Size() - endsWith.Size()) == endsWith;
		}

		template<typename IntoT>
		constexpr void SplitInto(this auto&& self, IntoT& into, CharT delimiter)
		{
			self.SplitInto(into, StringViewT(&delimiter, 1));
		}

		template<typename IntoT>
		constexpr void SplitInto(this auto&& self, IntoT& into, StringViewT delimiter)
		{
			U64 start = 0;
			U64 current = 0;
			while (current < self.Size())
			{
				if (self.SubStr(current, delimiter.Size()) == delimiter)
				{
					into.PushBack(self.SubStr(start, current - start));
					current += delimiter.Size();
					start = current;
				}
				else
				{
					current++;
				}
			}

			if (start < self.Size())
			{
				into.PushBack(self.SubStr(start));
			}
		}

		template<typename A, typename B>
			requires requires () {
				{ std::declval<A>().Data() } -> std::convertible_to<const CharT*>;
				{ std::declval<A>().Size() } -> std::convertible_to<U64>;
				{ std::declval<B>().Data() } -> std::convertible_to<const CharT*>;
				{ std::declval<B>().Size() } -> std::convertible_to<U64>;
		}
		[[nodiscard]] constexpr bool operator==(this const A& a, const B& b)
		{
			REX_CORE_TRACE_FUNC();
			if (a.Size() != b.Size())
				return false;

			if (a.Data() == b.Data())
				return true;

			if (a.Data() == nullptr || b.Data() == nullptr)
				return false;

			return StringCompare(a.Data(), b.Data(), a.Size()) == 0;
		}

		template<typename A>
			requires requires () {
				{ std::declval<A>().Data() } -> std::convertible_to<const CharT*>;
				{ std::declval<A>().Size() } -> std::convertible_to<U64>;
		}
		[[nodiscard]] constexpr bool operator==(this const A& a, const CharT* b)
		{
			REX_CORE_TRACE_FUNC();
			if (a.Data() == b)
				return true;

			if (b == nullptr)
				return a.Size() == 0;

			return (a.Size() == StringLength(b)) && StringCompare(a.Data(), b, a.Size()) == 0;
		}

		template<typename A, typename B>
			requires requires () {
				{ std::declval<A>().Data() } -> std::convertible_to<const CharT*>;
				{ std::declval<A>().Size() } -> std::convertible_to<U64>;
				{ std::declval<B>().Data() } -> std::convertible_to<const CharT*>;
				{ std::declval<B>().Size() } -> std::convertible_to<U64>;
		}
		[[nodiscard]] constexpr std::strong_ordering operator<=>(this const A& a, const B& b)
		{
			REX_CORE_TRACE_FUNC();
			if (a.Data() == b.Data())
				return std::strong_ordering::equal;
			
			if (a.Data() == nullptr)
				return std::strong_ordering::less;

			if (b.Data() == nullptr)
				return std::strong_ordering::greater;

			const S32 result = StringCompare(a.Data(), b.Data(), Math::Min(a.Size(), b.Size()));
			if (result < 0)
				return std::strong_ordering::less;
			else if (result > 0)
				return std::strong_ordering::greater;
			else
				return a.Size() <=> b.Size();
		}
	};

	template<typename CharT>
	class StringViewBase : public SpanTypeBase<const CharT, U64, StringViewBase<CharT>, StringViewBase<CharT>>
						 , public StringTypeBase<CharT, StringViewBase<CharT>, StringViewBase<CharT>>
	{
	public:
		using ConstIterator = const CharT*;
		static_assert(std::contiguous_iterator<ConstIterator>);


		REX_CORE_DEFAULT_COPY(StringViewBase);
		REX_CORE_DEFAULT_MOVE(StringViewBase);

		constexpr StringViewBase()
			: m_data(nullptr), m_size(0)
		{}

		constexpr StringViewBase(const CharT* data, U64 size)
			: m_data(data), m_size(size)
		{}

		constexpr StringViewBase(const CharT* nullTerminatedString)
			: m_data(nullTerminatedString), m_size(StringLength(nullTerminatedString))
		{}

		[[nodiscard]] constexpr U64 Size() const { return m_size; }
		[[nodiscard]] constexpr const CharT* Data() const { return m_data; }

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
		const CharT* m_data;
		U64 m_size;
	};

	using StringView = StringViewBase<char>;
	using WStringView = StringViewBase<wchar_t>;

	template<typename CharT, IAllocator Allocator, U64 InplaceSize = 0>
	class StringBase : public VectorTypeBase<CharT, U64, StringBase<CharT, Allocator, InplaceSize>>
					 , public StringTypeBase<CharT, StringViewBase<CharT>, StringBase<CharT, Allocator, InplaceSize>>
	{
	private:
		constexpr static U64 SmallStringSize = Math::Max((sizeof(U64) + sizeof(CharT*)) / sizeof(CharT), InplaceSize + 1);
		constexpr static U64 SmallStringBitMask = (1llu << 63llu);

		using BaseT = VectorTypeBase<CharT, U64, StringBase<CharT, Allocator, InplaceSize>>;

	public:
		using AllocatorType = Allocator;
		using StringViewType = StringViewBase<CharT>;
		constexpr static U64 InplaceCapacity = SmallStringSize - 1;

		REX_CORE_NO_COPY(StringBase);
		
		constexpr StringBase(StringBase&& other) noexcept
			: m_allocator(other.m_allocator), m_size(other.m_size)
		{
			MemCopy(&other.m_bigSmallUnion, &m_bigSmallUnion, sizeof(m_bigSmallUnion));

			MemSet(&other.m_bigSmallUnion, 0, sizeof(m_bigSmallUnion));
			other.m_size = SmallStringBitMask;
		}

		constexpr StringBase& operator=(StringBase&& other) noexcept {
			m_allocator = other.m_allocator;
			m_size = other.m_size;
			MemCopy(&other.m_bigSmallUnion, &m_bigSmallUnion, sizeof(m_bigSmallUnion));

			MemSet(&other.m_bigSmallUnion, 0, sizeof(m_bigSmallUnion));
			other.m_size = SmallStringBitMask;
			return *this;
		}

		constexpr StringBase(AllocatorRef<Allocator> allocator = AllocatorRefDefaultArg<Allocator>()) noexcept
			: m_allocator(allocator)
		{}

		explicit constexpr StringBase(BaseT::SpanType from, AllocatorRef<Allocator> allocator = AllocatorRefDefaultArg<Allocator>())
			: m_allocator(allocator)
		{
			REX_CORE_TRACE_FUNC();
			Reserve(from.Size());
			MemCopy(from.Data(), Data(), from.Size() * sizeof(CharT));
			SetSize(from.Size());
		}

		explicit constexpr StringBase(const CharT* nullTerminatedString, AllocatorRef<Allocator> allocator = AllocatorRefDefaultArg<Allocator>())
			: m_allocator(allocator)
		{
			REX_CORE_TRACE_FUNC();
			const U64 length = StringLength(nullTerminatedString);
			Reserve(length);
			MemCopy(nullTerminatedString, Data(), length * sizeof(CharT));
			SetSize(length); // Will add the null terminator
		}

		explicit constexpr StringBase(StringViewType from, AllocatorRef<Allocator> allocator = AllocatorRefDefaultArg<Allocator>())
			: m_allocator(allocator)
		{
			REX_CORE_TRACE_FUNC();
			Reserve(from.Size());
			MemCopy(from.Data(), Data(), from.Size() * sizeof(CharT));
			SetSize(from.Size());
		}

		[[nodiscard]] constexpr const CharT* Data() const { return IsSmallString() ? m_bigSmallUnion.m_small : m_bigSmallUnion.m_big.m_data; }
		[[nodiscard]] constexpr CharT* Data() { return IsSmallString() ? m_bigSmallUnion.m_small : m_bigSmallUnion.m_big.m_data; }
		[[nodiscard]] constexpr const CharT* CStr() const { return Data(); }
		[[nodiscard]] constexpr U64 Size() const { return m_size & (~SmallStringBitMask); }
		[[nodiscard]] constexpr U64 Capacity() const { return IsSmallString() ? SmallStringSize - 1 : m_bigSmallUnion.m_big.m_capacity; } // -1 for null terminator
		[[nodiscard]] constexpr AllocatorRef<Allocator> GetAllocator() const { return m_allocator; }

		constexpr void Reserve(U64 newCapacity)
		{
			REX_CORE_TRACE_FUNC();
			if (newCapacity <= Capacity())
				return;

			if (IsSmallString())
			{
				CharT* newData = static_cast<CharT*>(m_allocator.Allocate((newCapacity + 1) * sizeof(CharT), alignof(CharT)));
				MemCopy(m_bigSmallUnion.m_small, newData, (Size() + 1) * sizeof(CharT));
				SetSmallString(false);
				m_bigSmallUnion.m_big.m_data = newData;
				m_bigSmallUnion.m_big.m_capacity = newCapacity;
			}
			else
			{
				m_bigSmallUnion.m_big.m_data = static_cast<CharT*>(m_allocator.Reallocate(m_bigSmallUnion.m_big.m_data, (m_bigSmallUnion.m_big.m_capacity + 1) * sizeof(CharT), (newCapacity + 1) * sizeof(CharT), alignof(CharT)));
				m_bigSmallUnion.m_big.m_capacity = newCapacity;
			}
		}

		constexpr void Resize(U64 newSize, CharT newCharsValue = '\0')
		{
			REX_CORE_TRACE_FUNC();
			if (newSize == 0)
			{
				Free();
			}
			else if (newSize < Size())
			{
				if (IsSmallString())
				{
					SetSize(newSize);
				}
				else
				{
					// TODO perf : use Reallocate
					CharT* oldData = m_bigSmallUnion.m_big.m_data; // Because m_bigSmallUnion.m_small will overwrite m_bigSmallUnion.m_big.m_data
					CharT* newData = newSize < SmallStringSize ? m_bigSmallUnion.m_small : static_cast<CharT*>(m_allocator.Allocate((newSize + 1) * sizeof(CharT), alignof(CharT)));

					auto oldCapacity = m_bigSmallUnion.m_big.m_capacity;
					MemCopy(oldData, newData, (newSize + 1) * sizeof(CharT));
					m_allocator.Free(oldData, (oldCapacity + 1) * sizeof(CharT));


					if (newSize < SmallStringSize)
					{
						SetSmallString(true);
					}
					else
					{
						m_bigSmallUnion.m_big.m_data = newData;
						m_bigSmallUnion.m_big.m_capacity = newSize;
					}

					SetSize(newSize);
				}
			}
			else if (newSize > Size())
			{
				Reserve(newSize);
				for (U64 i = Size(); i < newSize; i++)
					Data()[i] = newCharsValue;

				SetSize(newSize);
			}
		}

		constexpr void Free()
		{
			REX_CORE_TRACE_FUNC();
			Base::Clear();
			if (!IsSmallString())
			{
				m_allocator.Free(m_bigSmallUnion.m_big.m_data, (m_bigSmallUnion.m_big.m_capacity + 1) * sizeof(CharT));
				// Go back to small string
				SetSmallString(true);
				SetSize(0);
			}
		}

		StringBase<CharT, Allocator, InplaceSize>& operator+=(StringViewType rhs)
		{
			REX_CORE_TRACE_FUNC();
			const auto newSize = Size() + rhs.Size();
			if (Capacity() <= newSize)
				Reserve(Internal::CalcGrowSize(Capacity(), newSize));
			
			MemCopy(rhs.Data(), Data() + Size(), rhs.Size() * sizeof(CharT));
			SetSize(newSize);
			return *this;
		}

		constexpr operator StringViewType() const { return StringViewType(Data(), Size()); }

	private:
		constexpr void SetSize(U64 size)
		{
			m_size = size | (m_size & SmallStringBitMask); // Preserve the small string bit
			Data()[size] = '\0';
		}

		constexpr bool IsSmallString() const
		{
			return m_size & SmallStringBitMask;
		}

		constexpr void SetSmallString(bool isSmall)
		{
			if (isSmall)
				m_size |= SmallStringBitMask;
			else
				m_size &= ~SmallStringBitMask;
		}


	private:
		// TODO perf : we could use part of m_size to store longer small strings, but watch out for endianness
		// The highest bit of m_size is used to determine if the string is small or not
		[[no_unique_address]] AllocatorRef<Allocator> m_allocator;
		U64 m_size = SmallStringBitMask;
		union {
			struct {
				U64 m_capacity;
				CharT* m_data;
			} m_big = {0, nullptr};
			CharT m_small[SmallStringSize];
		} m_bigSmallUnion;

		static_assert(sizeof(decltype(m_bigSmallUnion.m_big)) <= sizeof(decltype(m_bigSmallUnion.m_small)));

		using Base = VectorTypeBase<CharT, U64, StringBase<CharT, Allocator, InplaceSize>>;
		friend class Base;
	};

	template<typename StringT, typename StringViewT>
		requires requires () {
			{ std::declval<StringViewT>().Data() } -> std::convertible_to<const typename StringT::CharType*>;
			{ std::declval<StringViewT>().Size() } -> std::convertible_to<U64>;
	}
	inline StringT operator+(const StringT& lhs, const StringViewT& rhs)
	{
		REX_CORE_TRACE_FUNC();
		StringT result = [&] {
			if constexpr (requires { lhs.GetAllocator(); })
				return StringT(lhs.GetAllocator());
			else
				return StringT();
		}();
		result.Reserve(lhs.Size() + rhs.Size());
		result += lhs;
		result += rhs;
		return result;
	}

	template<typename StringT>
	inline StringT operator+(const StringT& lhs, const typename StringT::CharType* rhs)
	{
		REX_CORE_TRACE_FUNC();
		StringT result = [&] {
			if constexpr (requires { lhs.GetAllocator(); })
				return StringT(lhs.GetAllocator());
			else
				return StringT();
		}();
		result.Reserve(lhs.Size() + StringLength(rhs));
		result += lhs;
		result += rhs;
		return result;
	}

	template<IAllocator Allocator = DefaultAllocator>
	using String = StringBase<char, Allocator>;

	template<IAllocator Allocator = DefaultAllocator>
	using WString = StringBase<wchar_t, Allocator>;

	template<U64 InplaceSize, IAllocator Allocator = DefaultAllocator>
	using InplaceString = StringBase<char, Allocator, InplaceSize>;
	template<U64 InplaceSize, IAllocator Allocator = DefaultAllocator>
	using InplaceWString = StringBase<wchar_t, Allocator, InplaceSize>;

}

template <>
struct std::formatter<RexCore::StringView> : public std::formatter<std::string_view> {
	auto format(const RexCore::StringView& str, std::format_context& ctx) const {
		return std::formatter<std::string_view>::format(std::string_view(str.Data(), str.Size()), ctx);
	}
};

template <>
struct std::formatter<RexCore::WStringView, wchar_t> : public std::formatter<std::wstring_view, wchar_t> {
	auto format(const RexCore::WStringView& str, std::wformat_context& ctx) const {
		return std::formatter<std::wstring_view, wchar_t>::format(std::wstring_view(str.Data(), str.Size()), ctx);
	}
};

template <>
struct std::formatter<RexCore::String<>> : public std::formatter<std::string_view> {
	auto format(const RexCore::String<>& str, std::format_context& ctx) const {
		return std::formatter<std::string_view>::format(std::string_view(str.Data(), str.Size()), ctx);
	}
};

template <>
struct std::formatter<RexCore::WString<>, wchar_t> : public  std::formatter<std::wstring_view, wchar_t> {
	auto format(const RexCore::WString<>& str, std::wformat_context& ctx) const {
		return std::formatter<std::wstring_view, wchar_t>::format(std::wstring_view(str.Data(), str.Size()), ctx);
	}
};

inline std::ostream& operator<<(std::ostream& os, const RexCore::StringView& str)
{
	os << std::string_view(str.Data(), str.Size());
	return os;
}

inline std::wostream& operator<<(std::wostream& os, const RexCore::WStringView& str)
{
	os << std::wstring_view(str.Data(), str.Size());
	return os;
}

inline std::ostream& operator<<(std::ostream& os, const RexCore::String<>& str)
{
	os << std::string_view(str.Data(), str.Size());
	return os;
}

inline std::wostream& operator<<(std::wostream& os, const RexCore::WString<>& str)
{
	os << std::wstring_view(str.Data(), str.Size());
	return os;
}

template <>
struct ankerl::unordered_dense::hash<RexCore::StringView> {
	using is_avalanching = void;

	[[nodiscard]] RexCore::U64 operator()(const RexCore::StringView& str) const noexcept {
		return ankerl::unordered_dense::hash<std::string_view>{}(std::string_view(str.Data(), str.Size()));
	}
};

template <>
struct ankerl::unordered_dense::hash<RexCore::String<>> {
	using is_avalanching = void;

	[[nodiscard]] RexCore::U64 operator()(const RexCore::String<>& str) const noexcept {
		return ankerl::unordered_dense::hash<std::string_view>{}(std::string_view(str.Data(), str.Size()));
	}
};

template <>
struct ankerl::unordered_dense::hash<RexCore::WStringView> {
	using is_avalanching = void;

	[[nodiscard]] RexCore::U64 operator()(const RexCore::WStringView& str) const noexcept {
		return ankerl::unordered_dense::hash<std::wstring_view>{}(std::wstring_view(str.Data(), str.Size()));
	}
};

template <>
struct ankerl::unordered_dense::hash<RexCore::WString<>> {
	using is_avalanching = void;

	[[nodiscard]] RexCore::U64 operator()(const RexCore::WString<>& str) const noexcept {
		return ankerl::unordered_dense::hash<std::wstring_view>{}(std::wstring_view(str.Data(), str.Size()));
	}
};
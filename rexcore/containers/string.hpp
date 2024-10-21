#pragma once

#include <rexcore/containers/vector.hpp>
#include <rexcore/containers/span.hpp>
#include <rexcore/allocators.hpp>

#include <cstring>
#include <compare>

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

		template<typename A, typename B>
			requires requires () {
				{ std::declval<A>().Data() } -> std::convertible_to<const CharT*>;
				{ std::declval<A>().Size() } -> std::convertible_to<U64>;
				{ std::declval<B>().Data() } -> std::convertible_to<const CharT*>;
				{ std::declval<B>().Size() } -> std::convertible_to<U64>;
		}
		[[nodiscard]] constexpr bool operator==(this const A& a, const B& b)
		{
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

	public:
		using AllocatorType = Allocator;
		using StringViewType = StringViewBase<CharT>;
		constexpr static U64 InplaceCapacity = SmallStringSize - 1;

		REX_CORE_NO_COPY(StringBase);
		REX_CORE_DEFAULT_MOVE(StringBase);

		constexpr StringBase() noexcept = default;

		explicit constexpr StringBase(const CharT* nullTerminatedString)
		{
			const U64 length = StringLength(nullTerminatedString);
			Reserve(length);
			MemCopy(nullTerminatedString, Data(), length * sizeof(CharT));
			SetSize(length); // Will add the null terminator
		}

		[[nodiscard]] constexpr const CharT* Data() const { return IsSmallString() ? m_small : m_big.m_data; }
		[[nodiscard]] constexpr CharT* Data() { return IsSmallString() ? m_small : m_big.m_data; }
		[[nodiscard]] constexpr U64 Size() const { return m_size & (~SmallStringBitMask); }
		[[nodiscard]] constexpr U64 Capacity() const { return IsSmallString() ? SmallStringSize - 1 : m_big.m_capacity; } // -1 for null terminator

		constexpr void Reserve(U64 newCapacity)
		{
			if (newCapacity <= Capacity())
				return;

			if (IsSmallString())
			{
				CharT* newData = static_cast<CharT*>(m_allocator.Allocate((newCapacity + 1) * sizeof(CharT), alignof(CharT)));
				MemCopy(m_small, newData, (Size() + 1) * sizeof(CharT));
				SetSmallString(false);
				m_big.m_data = newData;
				m_big.m_capacity = newCapacity;
			}
			else
			{
				m_big.m_data = static_cast<CharT*>(m_allocator.Reallocate(m_big.m_data, (m_big.m_capacity + 1) * sizeof(CharT), (newCapacity + 1) * sizeof(CharT), alignof(CharT)));
				m_big.m_capacity = newCapacity;
			}
		}

		constexpr void Resize(U64 newSize, CharT newCharsValue = '\0')
		{
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
					CharT* oldData = m_big.m_data; // Because m_small will overwrite m_big.m_data
					CharT* newData = newSize < SmallStringSize ? m_small : static_cast<CharT*>(m_allocator.Allocate((newSize + 1) * sizeof(CharT), alignof(CharT)));

					auto oldCapacity = m_big.m_capacity;
					MemCopy(oldData, newData, (newSize + 1) * sizeof(CharT));
					m_allocator.Free(oldData, (oldCapacity + 1) * sizeof(CharT));


					if (newSize < SmallStringSize)
					{
						SetSmallString(true);
					}
					else
					{
						m_big.m_data = newData;
						m_big.m_capacity = newSize;
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
			Base::Clear();
			if (!IsSmallString())
			{
				m_allocator.Free(m_big.m_data, (m_big.m_capacity + 1) * sizeof(CharT));
				// Go back to small string
				SetSmallString(true);
				SetSize(0);
			}
		}

		StringBase<CharT, Allocator, InplaceSize>& operator+=(StringViewType rhs)
		{
			Reserve(Size() + rhs.Size());
			MemCopy(rhs.Data(), Data() + Size(), rhs.Size() * sizeof(CharT));
			SetSize(Size() + rhs.Size());
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
		Allocator m_allocator;
		U64 m_size = SmallStringBitMask;
		union {
			struct {
				U64 m_capacity;
				CharT* m_data;
			} m_big = {0, nullptr};
			CharT m_small[SmallStringSize];
		};

		static_assert(sizeof(decltype(m_big)) <= sizeof(decltype(m_small)));

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
		StringT result = StringT();
		result.Reserve(lhs.Size() + rhs.Size());
		result += lhs;
		result += rhs;
		return result;
	}

	template<typename StringT>
	inline StringT operator+(const StringT& lhs, const typename StringT::CharType* rhs)
	{
		StringT result = StringT();
		result.Reserve(lhs.Size() + StringLength(rhs));
		result += lhs;
		result += rhs;
		return result;
	}

	using String = StringBase<char, DefaultAllocator>;
	using WString = StringBase<wchar_t, DefaultAllocator>;

	template<U64 InplaceSize, IAllocator Allocator = DefaultAllocator>
	using InplaceString = StringBase<char, Allocator, InplaceSize>;
	template<U64 InplaceSize, IAllocator Allocator = DefaultAllocator>
	using InplaceWString = StringBase<wchar_t, Allocator, InplaceSize>;

}
#include <tests/test_utils.hpp>

#include <rexcore/containers/string.hpp>

#include <sstream>

using namespace RexCore;

TEST_CASE("Formatters/StringView")
{
	{
		StringView str("Hello, World!");
		std::string formatted = std::format("{}", str);
		ASSERT(formatted == "Hello, World!");
	}

	{
		StringView str("Hello, World!");
		std::ostringstream os;
		os << str;
		ASSERT(os.str() == "Hello, World!");
	}
}

TEST_CASE("Formatters/WStringView")
{
	{
		WStringView str(L"Hello, World!");
		std::wstring formatted = std::format(L"{}", str);
		ASSERT(formatted == L"Hello, World!");
	}

	{
		WStringView str(L"Hello, World!");
		std::wostringstream os;
		os << str;
		ASSERT(os.str() == L"Hello, World!");
	}
}

TEST_CASE("Formatters/String")
{
	{
		String<> str("Hello, World!");
		std::string formatted = std::format("{}", str);
		ASSERT(formatted == "Hello, World!");
	}

	{
		String str("Hello, World!");
		std::ostringstream os;
		os << str;
		ASSERT(os.str() == "Hello, World!");
	}
}

TEST_CASE("Formatters/WString")
{
	{
		WString str(L"Hello, World!");
		std::wstring formatted = std::format(L"{}", str);
		ASSERT(formatted == L"Hello, World!");
	}

	{
		WString str(L"Hello, World!");
		std::wostringstream os;
		os << str;
		ASSERT(os.str() == L"Hello, World!");
	}
}
#include <tests/test_utils.hpp>

#include <rexcore/containers/string.hpp>

using namespace RexCore;

TEST_CASE("Formatters/StringView")
{
	StringView str("Hello, World!");
	std::string formatted = std::format("{}", str);
	ASSERT(formatted == "Hello, World!");
}

TEST_CASE("Formatters/WStringView")
{
	WStringView str(L"Hello, World!");
	std::wstring formatted = std::format(L"{}", str);
	ASSERT(formatted == L"Hello, World!");
}

TEST_CASE("Formatters/String")
{
	String<> str("Hello, World!");
	std::string formatted = std::format("{}", str);
	ASSERT(formatted == "Hello, World!");
}

TEST_CASE("Formatters/WString")
{
	WString str(L"Hello, World!");
	std::wstring formatted = std::format(L"{}", str);
	ASSERT(formatted == L"Hello, World!");
}
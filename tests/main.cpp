#include <tests/test_utils.hpp>

int main()
{
	return RexCore::Tests::RegisterTestCase::RunAllTests() ? 0 : -1;
}
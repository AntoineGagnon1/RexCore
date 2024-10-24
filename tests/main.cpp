#include <tests/test_utils.hpp>

#include <rexcore/allocators.hpp>

int main()
{
	bool success = RexCore::Tests::RegisterTestCase::RunAllTests();
	success &= !RexCore::CheckForLeaks();
	return success ? 0 : -1;
}
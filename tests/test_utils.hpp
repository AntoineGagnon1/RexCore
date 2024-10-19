#pragma once

#include <vector>
#include <algorithm>
#include <stdexcept>
#include <format>

namespace RexCore::Tests
{
	class RegisterTestCase
	{
	public:
		RegisterTestCase(const char* name, void (*body)())
		{
			s_testCases.emplace_back(name, body);
		}

		static bool RunAllTests()
		{
			std::sort(s_testCases.begin(), s_testCases.end(), [](const TestCase& a, const TestCase& b) { return strcmp(a.name, b.name) < 0; });

			size_t testFailed = 0;
			for (const TestCase& testCase : s_testCases)
			{
				try 
				{
					testCase.body();
				}
				catch (const std::exception& e)
				{
					printf("%s [\x1B[31mFailed\033[0m]\n", testCase.name);
					printf("\t%s\n", e.what());
					testFailed++;
					continue;
				}
				
				printf("%s [\x1B[32mSuccess\033[0m]\n", testCase.name);
			}

			printf("\n\n");
			printf("Ran %llu asserts in %llu tests.\n", s_AssertCount, s_testCases.size());
			if (testFailed == 0)
				printf("\x1B[32mAll %llu tests passed!\033[0m\n", s_testCases.size());
			else
				printf("\x1B[31m%llu/%llu test failed\033[0m\n", testFailed, s_testCases.size());
			return testFailed == 0;
		}

		inline static size_t s_AssertCount = 0;

	private:
		struct TestCase
		{
			const char* name;
			void (*body)();
		};

		inline static std::vector<TestCase> s_testCases;
	};

#define CONCAT(a, b) CONCAT_INNER(a, b)
#define CONCAT_INNER(a, b) a ## b
#define DEBUG_BREAK() __debugbreak()

#define TEST_CASE_INNER(name, funcName) \
	static void funcName(); \
	static ::RexCore::Tests::RegisterTestCase CONCAT(test_, __COUNTER__) = ::RexCore::Tests::RegisterTestCase(name, funcName); \
	static void funcName()

#define TEST_CASE(name) TEST_CASE_INNER(name, CONCAT(testCase_, __COUNTER__))

#define ASSERT(cond) if (!(cond)) {DEBUG_BREAK(); throw std::runtime_error(std::format("{}:{}\t[{}]", __FILE__, __LINE__, #cond));} \
	::RexCore::Tests::RegisterTestCase::s_AssertCount++
}
#include <benchmarks/bench_utils.hpp>

#include <rexcore/core.hpp>
#include <rexcore/containers/string.hpp>
#include <rexcore/containers/vector.hpp>
#include <rexcore/containers/smart_ptrs.hpp>
#include <rexcore/allocators.hpp>

#include <vector>
#include <memory>
#include <string>

using namespace RexCore;

BENCHMARK("Containers/UniquePtr")
{
	BENCH_LOOP("UniquePtr", 1'000'000, 1, {
		UniquePtr<int> ptr = MakeUnique<int>(1);
	});

	BENCH_LOOP("std::unique_ptr", 1'000'000, 1, {
		std::unique_ptr<int> ptr = std::make_unique<int>(1);
	});
}

BENCHMARK("Containers/SharedPtr")
{
	SharedPtr<int> sharedPtr = MakeShared<int>(1);
	BENCH_LOOP("SharedPtr", 1'000'000, 1, {
		SharedPtr<int> copy = sharedPtr;
	});

	AtomicSharedPtr<int> atomicPtr = MakeAtomicShared<int>(1);
	BENCH_LOOP("AtomicSharedPtr", 1'000'000, 1, {
		AtomicSharedPtr<int> copy = atomicPtr;
	});

	std::shared_ptr<int> stdPtr = std::make_shared<int>(1);
	BENCH_LOOP("std::shared_ptr", 1'000'000, 1, {
		std::shared_ptr<int> copy = stdPtr;
	});
}

BENCHMARK("Containers/Function")
{
	Function<int(int)> func = [](int x) { return x + 1; };
	BENCH_LOOP("Function - Copy", 1'000'000, 1, {
		Function<int(int)> copy = func.Clone();
	});
	BENCH_LOOP("Function - Call", 1'000'000, 1, {
		func(1);
	});

	std::function<int(int)> stdFunc = [](int x) { return x + 1; };
	BENCH_LOOP("std::function - Copy", 1'000'000, 1, {
		std::function<int(int)> copy = stdFunc;
	});
	BENCH_LOOP("std::function - Call", 1'000'000, 1, {
		stdFunc(1);
	});
}

BENCHMARK("Containers/Vector")
{
	Vector<int> vec;
	BENCH_LOOP("Vector - PushBack", 1'000'000, 1, {
		vec.PushBack(1);
	});
	BENCH_LOOP("Vector - Foreach", 1'000, 1'000'000, {
		for (int& i : vec)
			i++;
	});
	BENCH_LOOP("Vector - PopBack", 1'000'000, 1, {
		vec.PopBack();
	});

	InplaceVector<int, 16> inplaceVec;
	BENCH_LOOP("InplaceVector - PushBack", 1'000'000, 1, {
		inplaceVec.PushBack(1);
		});
	BENCH_LOOP("InplaceVector - Foreach", 1'000, 1'000'000, {
		for (int& i : inplaceVec)
			i++;
		});
	BENCH_LOOP("InplaceVector - PopBack", 1'000'000, 1, {
		inplaceVec.PopBack();
	});

	std::vector<int> stdVec;
	BENCH_LOOP("std::vector - PushBack", 1'000'000, 1, {
		stdVec.push_back(1);
	});
	BENCH_LOOP("std::vector - Foreach", 1'000, 1'000'000, {
		for (int& i : stdVec)
			i++;
		});
	BENCH_LOOP("std::vector - PopBack", 1'000'000, 1, {
		stdVec.pop_back();
	});
}

BENCHMARK("Containers/String")
{
	String<> str;
	BENCH_LOOP("String - Append", 100'000, 1, {
		str += "Hello";
	});
	BENCH_LOOP("String - Copy", 10'000, 1, {
		String copy = str.Clone();
	});

	std::string stdStr;
	BENCH_LOOP("std::string - Append", 100'000, 1, {
		stdStr += "Hello";
	});
	BENCH_LOOP("std::string - Copy", 10'000, 1, {
		std::string copy = stdStr;
	});
}
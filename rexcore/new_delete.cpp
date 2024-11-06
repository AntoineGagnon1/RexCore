#include <rexcore/allocators.hpp>

#include <cstddef> // max_align_t

// https://timsong-cpp.github.io/cppwp/n4861/new.delete

[[nodiscard]] void* operator new(size_t size)
{
	void* ptr = RexCore::DefaultAllocator{}.Allocate(size, alignof(std::max_align_t));
	REX_CORE_ASSERT(ptr != nullptr);
	return ptr;
}

[[nodiscard]] void* operator new(std::size_t size, std::align_val_t alignment)
{
	void* ptr = RexCore::DefaultAllocator{}.Allocate(size, static_cast<RexCore::U64>(alignment));
	REX_CORE_ASSERT(ptr != nullptr);
	return ptr;
}

[[nodiscard]] void* operator new(std::size_t size, const std::nothrow_t&) noexcept
{
	void* ptr = RexCore::DefaultAllocator{}.Allocate(size, alignof(std::max_align_t));
	return ptr;
}

[[nodiscard]] void* operator new(std::size_t size, std::align_val_t alignment, const std::nothrow_t&) noexcept
{
	void* ptr = RexCore::DefaultAllocator{}.Allocate(size, static_cast<RexCore::U64>(alignment));
	return ptr;
}

void operator delete(void* ptr) noexcept
{
	RexCore::DefaultAllocator{}.FreeNoSize(ptr);
}

void operator delete(void* ptr, size_t size) noexcept
{
	RexCore::DefaultAllocator{}.Free(ptr, static_cast<RexCore::U64>(size));
}

void operator delete(void* ptr, [[maybe_unused]] std::align_val_t alignment) noexcept
{
	RexCore::DefaultAllocator{}.FreeNoSize(ptr);
}

void operator delete(void* ptr, std::size_t size, [[maybe_unused]] std::align_val_t alignment) noexcept
{
	RexCore::DefaultAllocator{}.Free(ptr, static_cast<RexCore::U64>(size));
}

void operator delete(void* ptr, const std::nothrow_t&) noexcept
{
	RexCore::DefaultAllocator{}.FreeNoSize(ptr);
}

void operator delete(void* ptr, [[maybe_unused]] std::align_val_t alignment, const std::nothrow_t&) noexcept
{
	RexCore::DefaultAllocator{}.FreeNoSize(ptr);
}

[[nodiscard]] void* operator new[](size_t size)
{
	void* ptr = RexCore::DefaultAllocator{}.Allocate(size, alignof(std::max_align_t));
	REX_CORE_ASSERT(ptr != nullptr);
	return ptr;
}

[[nodiscard]] void* operator new[](std::size_t size, std::align_val_t alignment)
{
	void* ptr = RexCore::DefaultAllocator{}.Allocate(size, static_cast<RexCore::U64>(alignment));
	REX_CORE_ASSERT(ptr != nullptr);
	return ptr;
}

[[nodiscard]] void* operator new[](std::size_t size, const std::nothrow_t&) noexcept
{
	void* ptr = RexCore::DefaultAllocator{}.Allocate(size, alignof(std::max_align_t));
	return ptr;
}

[[nodiscard]] void* operator new[](std::size_t size, std::align_val_t alignment, const std::nothrow_t&) noexcept
{
	void* ptr = RexCore::DefaultAllocator{}.Allocate(size, static_cast<RexCore::U64>(alignment));
	return ptr;
}

void operator delete[](void* ptr) noexcept
{
	RexCore::DefaultAllocator{}.FreeNoSize(ptr);
}

void operator delete[](void* ptr, size_t size) noexcept
{
	RexCore::DefaultAllocator{}.Free(ptr, static_cast<RexCore::U64>(size));
}

void operator delete[](void* ptr, [[maybe_unused]] std::align_val_t alignment) noexcept
{
	RexCore::DefaultAllocator{}.FreeNoSize(ptr);
}

void operator delete[](void* ptr, std::size_t size, [[maybe_unused]] std::align_val_t alignment) noexcept
{
	RexCore::DefaultAllocator{}.Free(ptr, static_cast<RexCore::U64>(size));
}

void operator delete[](void* ptr, const std::nothrow_t&) noexcept
{
	RexCore::DefaultAllocator{}.FreeNoSize(ptr);
}

void operator delete[](void* ptr, [[maybe_unused]] std::align_val_t alignment, const std::nothrow_t&) noexcept
{
	RexCore::DefaultAllocator{}.FreeNoSize(ptr);
}
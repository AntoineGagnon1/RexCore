#pragma once

namespace RexCore
{
#define REX_CORE_NO_COPY(T) \
	T(const T&) = delete; \
	T& operator=(const T&) = delete

#define REX_CORE_NO_MOVE(T) \
	T(T&&) = delete; \
	T& operator=(T&&) = delete

#define REX_CORE_DEFAULT_MOVE(T) \
	T(T&&) noexcept = default; \
	T& operator=(T&&) noexcept = default
}
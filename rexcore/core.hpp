#pragma once
#include <rexcore/vendors/Superluminal/PerformanceAPI.h>

#include <stdint.h>
#include <source_location>
#include <format>

namespace RexCore
{
	using U8 = uint8_t;
	using U16 = uint16_t;
	using U32 = uint32_t;
	using U64 = uint64_t;

	using S8 = int8_t;
	using S16 = int16_t;
	using S32 = int32_t;
	using S64 = int64_t;

	using Byte = U8;

#define REX_CORE_NO_COPY(T) \
	T(const T&) = delete; \
	T& operator=(const T&) = delete

#define REX_CORE_NO_MOVE(T) \
	T(T&&) = delete; \
	T& operator=(T&&) = delete

#define REX_CORE_DEFAULT_MOVE(T) \
	T(T&&) noexcept = default; \
	T& operator=(T&&) noexcept = default

#define REX_CORE_DEFAULT_COPY(T) \
	T(const T&) = default; \
	T& operator=(const T&) = default


#ifdef REX_CORE_TRACE_ENABLED
#define REX_CORE_TRACE_FUNC() PERFORMANCEAPI_INSTRUMENT(__FUNCTION__)
#define REX_CORE_TRACE_NAMED(name) PERFORMANCEAPI_INSTRUMENT(name)
#else
#define REX_CORE_TRACE_FUNC()
#define REX_CORE_TRACE_NAMED(name)
#endif
}

template <>
struct std::formatter<std::source_location> {
	constexpr auto parse(std::format_parse_context& ctx) const {
		return ctx.begin();
	}

	template <typename FormatContext>
	auto format(const std::source_location& loc, FormatContext& ctx) const {
		return std::format_to(ctx.out(), "{}::{}:{}", loc.file_name(), loc.function_name(), loc.line());
	}
};
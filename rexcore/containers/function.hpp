#pragma once
#include <rexcore/core.hpp>
#include <rexcore/allocators.hpp>

#pragma warning(push, 0)
#include <rexcore/vendors/skarupke_function.hpp>
#pragma warning(pop)

namespace RexCore
{
	template<typename R, typename ...Args>
	class Function;

	template<typename R, typename ...Args>
	class Function<R(Args...)> : func::function<R(Args...)>
	{
		using Impl = func::function<R(Args...)>;

	public:
		REX_CORE_NO_COPY(Function);
		REX_CORE_DEFAULT_MOVE(Function);

		constexpr Function() noexcept = default;
		constexpr Function(std::nullptr_t) noexcept : Impl(nullptr) {}
		
		constexpr Function Clone() const
		{
			return Function(*static_cast<const Impl*>(this));
		}

		template<typename T>
			requires func::detail::is_valid_function_argument<T, R(Args...)>::value
		constexpr Function(T functor)
			: Impl(std::move(functor))
		{
			static_assert(std::is_invocable_r_v<R, T, Args...>, "Invalid functor type");
			static_assert(sizeof(T) <= sizeof(func::detail::functor_padding), "Functor too big for inline allocator, use Function::Allocate()");
		}

		template<IAllocator Allocator, typename T>
			requires func::detail::is_valid_function_argument<T, R(Args...)>::value
		constexpr static Function Allocate(T functor, AllocatorRef<Allocator> allocator = AllocatorRefDefaultArg<Allocator>())
		{
			StdAllocatorAdaptor<T, Allocator> stdAllocator(allocator);
			return Function(std::move(functor), stdAllocator);
		}

		using Impl::operator();
		using Impl::operator bool;

	private:
		constexpr Function(const Impl& impl) : Impl(impl) {}

		template<typename T, typename Allocator>
		constexpr Function(T functor, const Allocator& allocator)
			: Impl(std::allocator_arg_t{}, allocator, std::move(functor))
		{}
	};

	template<typename T>
	inline bool operator==(std::nullptr_t, const Function<T>& rhs) noexcept
	{
		return !rhs;
	}
	template<typename T>
	inline bool operator==(const Function<T>& lhs, std::nullptr_t) noexcept
	{
		return !lhs;
	}
	template<typename T>
	inline bool operator!=(std::nullptr_t, const Function<T>& rhs) noexcept
	{
		return rhs;
	}
	template<typename T>
	inline bool operator!=(const Function<T>& lhs, std::nullptr_t) noexcept
	{
		return lhs;
	}
}
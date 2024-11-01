#pragma once
#include <rexcore/core.hpp>
#include <rexcore/allocators.hpp>

#pragma warning(push, 0)
#include <rexcore/vendors/flat_hash_map.hpp>
#pragma warning(pop)

namespace RexCore
{
	template<typename Key, IAllocator Allocator = DefaultAllocator>
	class HashSet : ska::flat_hash_set<Key, std::hash<Key>, std::equal_to<Key>, StdAllocatorAdaptor<Key, Allocator>>
	{
	private:
		using Impl = ska::flat_hash_set<Key, std::hash<Key>, std::equal_to<Key>, StdAllocatorAdaptor<Key, Allocator>>;

	public:
		using Iterator = typename Impl::iterator;
		using ConstIterator = typename Impl::const_iterator;

		REX_CORE_NO_COPY(HashSet);
		REX_CORE_DEFAULT_MOVE(HashSet);

		HashSet(AllocatorRef<Allocator> allocator = AllocatorRefDefaultArg<Allocator>())
			: Impl(StdAllocatorAdaptor<Key, Allocator>(allocator))
		{}

		[[nodiscard]] HashSet Clone() const
		{
			static_assert(IClonable<Key>, "The key type must be IClonable in order to clone a HashSet");
			return HashSet(*static_cast<const Impl*>(this));
		}

		[[nodiscard]] U64 Size() const { return Impl::size(); }
		[[nodiscard]] bool IsEmpty() const { return Size() == 0; }

		[[nodiscard]] bool Contains(const Key& key)
		{
			return Impl::find(key) != Impl::end();
		}

		void Reserve(U64 size) { Impl::reserve(size); }

		template<typename ...Args>
		bool Insert(Args&& ...args)
		{
			return Impl::emplace(std::forward<Args>(args)...).second;
		}

		bool Erase(const Key& key) { return Impl::erase(key) == 1; }

		void Clear() { Impl::clear(); }

		friend [[nodiscard]] bool operator==(const HashSet& lhs, const HashSet& rhs)
		{
			return static_cast<const Impl&>(lhs) == static_cast<const Impl&>(rhs);
		}

		friend [[nodiscard]] bool operator!=(const HashSet& lhs, const HashSet& rhs)
		{
			return !(lhs == rhs);
		}

		AllocatorRef<Allocator> GetAllocator() const
		{
			return Impl::get_allocator().GetAllocator();
		}

		[[nodiscard]]auto Begin() { return Impl::begin(); }
		[[nodiscard]]auto End() { return Impl::end(); }
		[[nodiscard]]auto Begin() const { return Impl::begin(); }
		[[nodiscard]]auto End() const { return Impl::end(); }
		[[nodiscard]]auto CBegin() const { return Impl::cbegin(); }
		[[nodiscard]]auto CEnd() const { return Impl::cend(); }

	public:
		[[nodiscard]] auto begin() { return Impl::begin(); }
		[[nodiscard]] auto end() { return Impl::end(); }
		[[nodiscard]] auto begin() const { return Impl::begin(); }
		[[nodiscard]] auto end() const { return Impl::end(); }
		[[nodiscard]] auto cbegin() const { return Impl::cbegin(); }
		[[nodiscard]] auto cend() const { return Impl::cend(); }

	private:
		HashSet(const Impl& impl)
			: Impl(impl)
		{}
	};
}
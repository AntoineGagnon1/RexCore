#pragma once
#include <rexcore/core.hpp>
#include <rexcore/allocators.hpp>
#include <rexcore/concepts.hpp>

#pragma warning(push, 0)
#include <stdexcept>
#include <rexcore/vendors/bytell_hash_map.hpp>
#pragma warning(pop)

namespace RexCore
{
	template<typename Key, typename Value, IAllocator Allocator = DefaultAllocator>
	class HashMap : ska::bytell_hash_map<Key, Value, std::hash<Key>, std::equal_to<Key>, StdAllocatorAdaptor<std::pair<Key,Value>, Allocator>>
	{
	private:
		using Impl = ska::bytell_hash_map<Key, Value, std::hash<Key>, std::equal_to<Key>, StdAllocatorAdaptor<std::pair<Key, Value>, Allocator>>;

	public:
		using Iterator = typename Impl::iterator;
		using ConstIterator = typename Impl::const_iterator;

		REX_CORE_NO_COPY(HashMap);
		REX_CORE_DEFAULT_MOVE(HashMap);

		HashMap(AllocatorRef<Allocator> allocator = AllocatorRefDefaultArg<Allocator>())
			: Impl(StdAllocatorAdaptor<std::pair<Key, Value>, Allocator>(allocator))
		{}

		[[nodiscard]] HashMap Clone() const
		{
			static_assert(IClonable<Key> && IClonable<Value>, "The value and key types must be IClonable in order to clone a HashMap");
			return HashMap(*static_cast<const Impl*>(this));
		}

		[[nodiscard]] U64 Size() const { return Impl::size(); }
		[[nodiscard]] bool IsEmpty() const { return Size() == 0; }

		Value& operator[](const Key& key) { return Impl::operator[](key); }
		Value& operator[](Key&& key) { return Impl::operator[](std::move(key)); }

		Value& At(const Key& key) {
			auto found = Impl::find(key);
			REX_CORE_ASSERT(found != Impl::end(), "Value not found ! use Find() instead");
			return found->second;
		}

		const Value& At(const Key& key) const {
			auto found = Impl::find(key);
			REX_CORE_ASSERT(found != Impl::end(), "Value not found ! use Find() instead");
			return found->second;
		}

		[[nodiscard]] Iterator Find(const Key& key) { return Impl::find(key); }
		[[nodiscard]] ConstIterator Find(const Key& key) const { return Impl::find(key); }

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

		template<typename ...Args>
		void InsertOrAssign(const Key& key, Args&& ...args)
		{
			Impl::insert_or_assign(key, std::forward<Args>(args)...);
		}

		bool Erase(const Key& key) { return Impl::erase(key) == 1; }

		void Clear() { Impl::clear(); }

		friend [[nodiscard]] bool operator==(const HashMap& lhs, const HashMap& rhs)
		{
			return static_cast<const Impl&>(lhs) == static_cast<const Impl&>(rhs);
		}

		friend [[nodiscard]] bool operator!=(const HashMap& lhs, const HashMap& rhs)
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
		HashMap(const Impl& impl)
			: Impl(impl)
		{}
	};
}
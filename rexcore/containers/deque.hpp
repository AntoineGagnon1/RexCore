#pragma once

#include <rexcore/allocators.hpp>
#include <rexcore/math.hpp>
#include <rexcore/containers/vector.hpp>

namespace RexCore 
{
	// Pointer stability : Always
	// Iterators invalidation : when the size changes (resize, push, pop, ...)
	template<typename IndexT, typename T, IAllocator Allocator>
	class DequeBase
	{
		struct Block;
	public:
		using AllocatorType = Allocator;
		using ValueType = T;
		using IndexType = IndexT;

		template<typename ValT>
		class IteratorBase
		{
		public:
			using RefType = std::add_lvalue_reference_t<ValT>;
			using PtrType = std::add_pointer_t<ValT>;

			// For std
			using difference_type = ptrdiff_t;
			using value_type = ValT;

			IteratorBase() noexcept = default;

			IteratorBase(Block** block, IndexT indexInBlock) noexcept
				: m_block(block), m_indexInBlock(indexInBlock)
			{}

			[[nodiscard]] friend bool operator==(const IteratorBase& lhs, const IteratorBase& rhs)
			{
				return lhs.m_block == rhs.m_block && lhs.m_indexInBlock == rhs.m_indexInBlock;
			}
			[[nodiscard]] friend bool operator!=(const IteratorBase& lhs, const IteratorBase& rhs)
			{
				return !(lhs == rhs);
			}

			IteratorBase& operator++()
			{
				m_indexInBlock++;
				if (m_indexInBlock == BlockSize)
				{
					m_block++;
					m_indexInBlock = 0;
				}
				return *this;
			}
			IteratorBase operator++(int)
			{
				IteratorBase copy(*this);
				++*this;
				return copy;
			}

			[[nodiscard]] RefType operator*() const
			{
				return (*m_block)->data[m_indexInBlock];
			}
			[[nodiscard]] PtrType operator->() const
			{
				return &(*m_block)->data[m_indexInBlock];
			}

			[[nodiscard]] operator IteratorBase<const ValT>() const
			{
				return { m_block, m_indexInBlock };
			}

		private:
			Block** m_block = nullptr;
			IndexT m_indexInBlock = 0;
		};

		using Iterator = IteratorBase<T>;
		using ConstIterator = IteratorBase<const T>;
		static_assert(std::forward_iterator<Iterator>);
		static_assert(std::forward_iterator<ConstIterator>);

	public:
		REX_CORE_NO_COPY(DequeBase);
		REX_CORE_DEFAULT_MOVE(DequeBase);

		constexpr explicit DequeBase(AllocatorRef<Allocator> allocator = AllocatorRefDefaultArg<Allocator>()) noexcept
			: m_allocator(allocator), m_blocks(allocator)
		{}

		constexpr ~DequeBase()
		{
			Free();
		}

		[[nodiscard]] constexpr auto operator[](this auto&& self, IndexT index) -> CopyConst<decltype(self), T>&
		{
			REX_CORE_ASSERT(index < self.m_size);
			return self.m_blocks[(self.m_start + index) / BlockSize]->data[(self.m_start + index) % BlockSize];
		}

		[[nodiscard]] constexpr bool IsEmpty() const { return m_size == 0; }
		[[nodiscard]] constexpr IndexT Size() const { return m_size; }
		[[nodiscard]] constexpr IndexT Capacity() const { return m_capacity; }

		[[nodiscard]] constexpr decltype(auto) First(this auto&& self)
		{
			REX_CORE_ASSERT(self.m_size > 0);
			return self.operator[](0);
		}

		[[nodiscard]] constexpr decltype(auto) Last(this auto&& self)
		{
			REX_CORE_ASSERT(self.m_size > 0);
			return self.operator[](self.m_size - 1);
		}

		[[nodiscard]] constexpr bool Contains(const T& value) const
		{
			for (const T& found : *this)
			{
				if (found == value)
					return true;
			}
			return false;
		}

		// Will return nullptr if not found
		[[nodiscard]] constexpr auto TryFind(this auto&& self, const T& value) -> CopyConst<decltype(self), T>*
		{
			for (CopyConst<decltype(self), T>& found : self)
			{
				if (found == value)
					return &found;
			}
			return nullptr;
		}

		// Will return nullptr if not found
		template<IPredicate<const T&> Predicate>
		[[nodiscard]] constexpr auto TryFind(this auto&& self, Predicate&& predicate) -> CopyConst<decltype(self), T>*
		{
			for (CopyConst<decltype(self), T>& found : self)
			{
				if (predicate(found))
					return &found;
			}
			return nullptr;
		}

		[[nodiscard]] DequeBase Clone() const
		{
			static_assert(IClonable<T>, "The value type must be IClonable in order to clone a deque");
			DequeBase clone(m_allocator);
			clone.m_blocks.Reserve(m_blocks.Size());
			clone.m_start = m_start;
			clone.m_size = m_size;
			clone.m_capacity = m_capacity;

			for (IndexT blockIndex = 0; blockIndex < m_blocks.Size(); blockIndex++)
			{
				Block* newBlock = static_cast<Block*>(clone.m_allocator.Allocate(sizeof(Block), alignof(T)));
				clone.m_blocks.EmplaceBack(newBlock);

				if constexpr (std::is_trivially_copyable_v<T>)
				{
					MemCopy(m_blocks[blockIndex]->data, newBlock->data, BlockSize * sizeof(T));
				}
				else
				{
					for (IndexT index = (blockIndex == 0 ? m_start : 0); index < BlockSize; index++)
					{
						RexCore::CloneInto(m_blocks[blockIndex]->data[index], &newBlock->data[index]);
					}
				}
			}

			return clone;
		}

		[[nodiscard]] constexpr Iterator Begin()
		{
			if (IsEmpty())
				return Iterator(nullptr, 0);

			return Iterator(&m_blocks.First(), m_start);
		}

		[[nodiscard]] constexpr ConstIterator Begin() const
		{
			if (IsEmpty())
				return ConstIterator(nullptr, 0);

			return ConstIterator(const_cast<Block**>(&m_blocks.First()), m_start);
		}

		[[nodiscard]] constexpr ConstIterator CBegin() const { return Begin(); }

		[[nodiscard]] constexpr Iterator End()
		{
			if (IsEmpty())
				return Iterator(nullptr, 0);

			return Iterator(&m_blocks.Last(), (m_start + m_size) % BlockSize);
		}

		[[nodiscard]] constexpr ConstIterator End() const
		{
			if (IsEmpty())
				return ConstIterator(nullptr, 0);

			return ConstIterator(const_cast<Block**>(&m_blocks.Last()), (m_start + m_size) % BlockSize);
		}

		[[nodiscard]] constexpr ConstIterator CEnd() const { return End(); }

		constexpr void Clear()
		{
			for (T& value : *this)
				value.~T();
			
			for (Block* block : m_blocks)
			{
				AddBlockToFreeList(block);
			}
			m_blocks.Clear();

			m_size = 0;
			m_start = 0;
			m_capacity = 0;
		}

		constexpr void ShrinkToFit()
		{
			Block* ptr = m_freeList;
			while (ptr != nullptr)
			{
				Block* temp = ptr;
				ptr = ptr->next;
				m_allocator.Free(temp, sizeof(Block));
			}

			m_freeList = nullptr;
		}

		constexpr void Free()
		{
			Clear();
			ShrinkToFit();
		}

		constexpr void Reserve(IndexT newCapacity)
		{
			if (newCapacity <= m_capacity)
				return;

			IndexT blocksNeeded = Math::CeilDiv<IndexT>(newCapacity - m_capacity, BlockSize);
			for (IndexT i = 0; i < blocksNeeded; i++)
			{
				AddBlockToFreeList(AllocateBlock());
			}
		}

		template<typename ...Args>
		constexpr void Resize(IndexT newSize, Args&& ...constructorArgs)
		{
			if (newSize == 0)
			{
				Free();
			}
			else if (newSize < m_size)
			{
				IndexT toRemove = m_size - newSize;
				for (IndexT i = 0; i < toRemove; i++)
				{
					[[maybe_unused]] auto _ = PopBack();
				}
			}
			else if (newSize > m_size)
			{
				Reserve(newSize);
				IndexT toAdd = newSize - m_size;
				for (IndexT i = 0; i < toAdd; i++)
				{
					EmplaceBack(std::forward<Args>(constructorArgs)...);
				}
			}
		}

		constexpr void PushBack(const T& value)
		{
			static_assert(IClonable<T>, "The value type must be IClonable in order to PushBack into a deque");

			IndexT newBlockIndex = (m_start + m_size) / BlockSize;
			IndexT newIndexInBlock = (m_start + m_size) % BlockSize;

			if (newBlockIndex >= m_blocks.Size())
			{
				m_blocks.EmplaceBack(GetBlock());
			}

			Block* block = m_blocks[newBlockIndex];
			RexCore::CloneInto(value, &block->data[newIndexInBlock]);
			m_size++;
		}

		template<typename ...Args>
		constexpr T& EmplaceBack(Args&& ...constructorArgs)
		{
			IndexT newBlockIndex = (m_start + m_size) / BlockSize;
			IndexT newIndexInBlock = (m_start + m_size) % BlockSize;

			if (newBlockIndex >= m_blocks.Size())
			{
				m_blocks.EmplaceBack(GetBlock());
			}

			Block* block = m_blocks[newBlockIndex];
			new (&block->data[newIndexInBlock]) T(std::forward<Args>(constructorArgs)...);
			m_size++;
			return block->data[newIndexInBlock];
		}

		constexpr T PopBack()
		{
			REX_CORE_ASSERT(m_size > 0);

			T temp = std::move(Last());
			m_size--;

			IndexT newLastBlockIndex = (m_start + m_size) / BlockSize;
			if (newLastBlockIndex < m_blocks.Size() - 1)
			{
				AddBlockToFreeList(m_blocks.PopBack());
			}

			return temp;
		}

		constexpr void PushFront(const T& value)
		{
			static_assert(IClonable<T>, "The value type must be IClonable in order to PushFront into a deque");

			if (m_start == 0)
			{
				m_start = BlockSize - 1;
				m_blocks.InsertAt(0, GetBlock());
			}
			else
			{
				m_start--;
			}

			RexCore::CloneInto(value, &m_blocks.First()->data[m_start]);
			m_size++;
		}

		template<typename ...Args>
		constexpr T& EmplaceFront(Args&& ...constructorArgs)
		{
			if (m_start == 0)
			{
				m_start = BlockSize - 1;
				m_blocks.InsertAt(0, GetBlock());
			}
			else
			{
				m_start--;
			}

			new (&m_blocks.First()->data[m_start]) T(std::forward<Args>(constructorArgs)...);
			m_size++;
			return m_blocks.First()->data[m_start];
		}

		constexpr T PopFront()
		{
			REX_CORE_ASSERT(m_size > 0);

			T temp = std::move(First());
			m_size--;
			m_start++;

			if (m_start == BlockSize)
			{
				AddBlockToFreeList(m_blocks.First());
				m_blocks.RemoveAtOrdered(0);
				m_start = 0;
			}

			return temp;
		}
		
	public:
		// For ranged-based for and other STL functions
		[[nodiscard]] constexpr Iterator begin() { return Begin(); }
		[[nodiscard]] constexpr ConstIterator begin() const { return Begin(); }
		[[nodiscard]] constexpr Iterator end() { return End(); }
		[[nodiscard]] constexpr ConstIterator end() const { return End(); }
		[[nodiscard]] constexpr ConstIterator cbegin() const { return CBegin(); }
		[[nodiscard]] constexpr ConstIterator cend() const { return CEnd(); }

	private:
		constexpr static IndexT BlockSize = Math::PreviousPowerOfTwo(Math::Max<IndexT>(1, 4096 / sizeof(T)));

		struct Block
		{
			union 
			{
				T data[BlockSize];
				Block* next; // For the free block list
			};
		};

		void AddBlockToFreeList(Block* block)
		{
			block->next = m_freeList;
			m_freeList = block;
		}

		Block* AllocateBlock()
		{
			m_capacity += BlockSize;
			return static_cast<Block*>(m_allocator.Allocate(sizeof(Block), alignof(T)));
		}

		Block* GetBlock()
		{
			if (m_freeList != nullptr)
			{
				Block* block = m_freeList;
				m_freeList = m_freeList->next;
				return block;
			}
			else
			{
				return AllocateBlock();
			}
		}

		[[no_unique_address]] AllocatorRef<Allocator> m_allocator;
		VectorBase<Block*, IndexT, Allocator> m_blocks;
		Block* m_freeList = nullptr;
		IndexT m_start = 0;
		IndexT m_size = 0;
		IndexT m_capacity = 0;
	};


	template<typename T, IAllocator Allocator = DefaultAllocator>
	using SmallDeque = DequeBase<U16, T, Allocator>;

	template<typename T, IAllocator Allocator = DefaultAllocator>
	using Deque = DequeBase<U32, T, Allocator>;

	template<typename T, IAllocator Allocator = DefaultAllocator>
	using BigDeque = DequeBase<U64, T, Allocator>;
}
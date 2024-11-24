#pragma once

#include <rexcore/allocators.hpp>
#include <rexcore/math.hpp>
#include <rexcore/containers/vector.hpp>

namespace RexCore 
{
	// Pointer stability : always stable
	template<typename IndexT, typename T, IAllocator Allocator>
	class StackBase
	{
	public:
		using AllocatorType = Allocator;
		using ValueType = T;
		using IndexType = IndexT;

		constexpr explicit StackBase(AllocatorRef<Allocator> allocator = AllocatorRefDefaultArg<Allocator>()) noexcept
			: m_allocator(allocator)
		{}

		constexpr ~StackBase()
		{
			Clear();
			ShrinkToFit();
		}

		[[nodiscard]] constexpr bool IsEmpty() const { return m_size == 0; }
		[[nodiscard]] constexpr IndexT Size() const { return m_size; }
		[[nodiscard]] constexpr AllocatorRef<Allocator> GetAllocator() const { return m_allocator; }

		constexpr void Clear()
		{
			REX_CORE_TRACE_FUNC();
			const IndexT size = m_size;
			for (IndexT i = 0; i < size; i++)
			{
				[[maybe_unused]] auto _ = PopBack();
			}
		}

		constexpr void ShrinkToFit()
		{
			REX_CORE_TRACE_FUNC();
			IndexT currentChunkSize = m_currentChunkSize;
			ChunkMetadata* currentChunk = m_currentChunk;
			
			if (m_size != 0) // If the stack is not empty start to shrink at the next chunk
			{
				currentChunkSize *= 2;
				currentChunk = currentChunk->next;
				m_currentChunk->next = nullptr;
			}
			else
			{
				m_currentChunk = nullptr;
			}

			while (currentChunk != nullptr)
			{
				ChunkMetadata* next = currentChunk->next;
				m_allocator.Free(currentChunk, sizeof(ChunkMetadata) + sizeof(T) * (currentChunkSize - StartChunkSize));
				currentChunk = next;
				currentChunkSize *= 2;
			}
		}

		[[nodiscard]] constexpr StackBase Clone() const
		{
			static_assert(IClonable<T>, "The value type must be IClonable in order to clone a stack");
			REX_CORE_TRACE_FUNC();

			StackBase clone(m_allocator);
			clone.m_currentChunkSize = m_currentChunkSize;
			clone.m_indexInChunk = m_indexInChunk;
			clone.m_size = m_size;
			clone.m_currentChunk = nullptr;

			ChunkMetadata* currentChunk = m_currentChunk;
			IndexT currentChunkSize = Math::Max(m_currentChunkSize, StartChunkSize);
			ChunkMetadata* previousCloneChunk = nullptr;
			while (currentChunk != nullptr)
			{
				ChunkMetadata* newChunk = static_cast<ChunkMetadata*>(clone.m_allocator.Allocate(sizeof(ChunkMetadata) + sizeof(T) * (currentChunkSize - StartChunkSize), alignof(ChunkMetadata)));
				newChunk->next = previousCloneChunk;
				newChunk->previous = nullptr;

				if (previousCloneChunk != nullptr)
				{
					previousCloneChunk->previous = newChunk;
				}
				previousCloneChunk = newChunk;

				if (clone.m_currentChunk == nullptr)
				{
					clone.m_currentChunk = newChunk;
				}

				const IndexT numElementsToCopy = currentChunkSize == m_currentChunkSize ? m_indexInChunk + 1 : currentChunkSize;
				for (IndexT i = 0; i < numElementsToCopy; i++)
				{
					RexCore::CloneInto(currentChunk->data[i], &newChunk->data[i]);
				}

				currentChunk = currentChunk->previous;
				currentChunkSize /= 2;
			}

			return clone;
		}

		constexpr void PushBack(const T& value)
		{
			static_assert(IClonable<T>, "The value type must be IClonable in order to PushBack into a stack");
			REX_CORE_TRACE_FUNC();

			m_indexInChunk++;
			m_size++;

			if (m_indexInChunk >= m_currentChunkSize)
			{
				NextBlock();
			}

			RexCore::CloneInto(value, &m_currentChunk->data[m_indexInChunk]);
		}

		template<typename ...Args>
		constexpr T& EmplaceBack(Args&& ...constructorArgs)
		{
			REX_CORE_TRACE_FUNC();
			m_indexInChunk++;
			m_size++;

			if (m_indexInChunk >= m_currentChunkSize)
			{
				NextBlock();
			}

			new (&m_currentChunk->data[m_indexInChunk]) T(std::forward<Args>(constructorArgs)...);
			return m_currentChunk->data[m_indexInChunk];
		}

		[[nodiscard]] constexpr T PopBack()
		{
			REX_CORE_TRACE_FUNC();
			REX_CORE_ASSERT(m_size > 0);

			T temp = std::move(m_currentChunk->data[m_indexInChunk]);

			if (m_indexInChunk == 0)
			{
				PreviousBlock();
			}
			else
			{
				m_indexInChunk--;
			}

			m_size--;
			return temp;
		}

		[[nodiscard]] constexpr const T& Peek() const
		{
			REX_CORE_ASSERT(m_size > 0);
			return m_currentChunk->data[m_indexInChunk];
		}

		[[nodiscard]] constexpr T& Peek()
		{
			REX_CORE_ASSERT(m_size > 0);
			return m_currentChunk->data[m_indexInChunk];
		}

	private:
		constexpr void NextBlock()
		{
			REX_CORE_TRACE_FUNC();
			m_indexInChunk = 0;
			m_currentChunkSize = m_currentChunkSize == 0 ? StartChunkSize : m_currentChunkSize * 2;

			if (m_currentChunk != nullptr && m_currentChunk->next != nullptr)
			{
				m_currentChunk = m_currentChunk->next;
			}
			else 
			{
				ChunkMetadata* newChunk = static_cast<ChunkMetadata*>(m_allocator.Allocate(sizeof(ChunkMetadata) + sizeof(T) * (m_currentChunkSize - StartChunkSize), alignof(ChunkMetadata)));
				newChunk->previous = m_currentChunk;
				newChunk->next = nullptr;

				if (m_currentChunk != nullptr)
				{
					m_currentChunk->next = newChunk;
				}

				m_currentChunk = newChunk;
			}
		}

		constexpr void PreviousBlock()
		{
			REX_CORE_TRACE_FUNC();
			if (m_size < StartChunkSize)
				return; // Stay on the first chunk

			m_currentChunkSize = m_currentChunkSize / 2;
			m_indexInChunk = m_currentChunkSize - 1;

			m_currentChunk = m_currentChunk->previous;
		}

	private:
		constexpr static IndexT StartChunkSize = 16;
		struct ChunkMetadata
		{
			ChunkMetadata* previous;
			ChunkMetadata* next;
			T data[StartChunkSize]; // Will be more than StartChunkSize elements in chunks 2, 3, 4, ...
		};

		[[no_unique_address]] AllocatorRef<Allocator> m_allocator;
		ChunkMetadata* m_currentChunk = nullptr;
		IndexT m_currentChunkSize = 0;
		IndexT m_indexInChunk = 0;
		IndexT m_size = 0;
	};

	template<typename T, IAllocator Allocator = DefaultAllocator>
	using SmallStack = StackBase<U16, T, Allocator>;

	template<typename T, IAllocator Allocator = DefaultAllocator>
	using Stack = StackBase<U32, T, Allocator>;

	template<typename T, IAllocator Allocator = DefaultAllocator>
	using BigStack = StackBase<U64, T, Allocator>;
}
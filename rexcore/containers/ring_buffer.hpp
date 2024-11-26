#pragma once

#include <rexcore/allocators.hpp>

namespace RexCore 
{
	template<IAllocator Allocator = DefaultAllocator>
	class RingBuffer
	{
	public:
		using AllocatorType = Allocator;
		
		constexpr explicit RingBuffer(U64 bufferSize, AllocatorRef<Allocator> allocator = AllocatorRefDefaultArg<Allocator>()) noexcept
			: m_allocator(allocator), m_size(bufferSize)
		{
			m_buffer = static_cast<Byte*>(m_allocator.Allocate(bufferSize, alignof(std::max_align_t)));
		}

		constexpr ~RingBuffer()
		{
			m_allocator.Free(m_buffer, m_size);
		}

		void* Allocate(U64 size, U64 alignment)
		{
			const U64 newEndPos = m_position + size + GetAlignmentPadding(m_buffer + m_position, alignment);
			if (newEndPos >= m_size)
			{
				const U64 padding = GetAlignmentPadding(m_buffer, alignment);
				m_position = size + padding;
				return m_buffer + padding;
			}
			else
			{
				void* newPtr = m_buffer + (newEndPos - size);
				m_position = newEndPos;
				return newPtr;
			}
		}

	private:

		constexpr U64 GetAlignmentPadding(void* ptr, U64 alignment)
		{
			auto misaligned = uintptr_t(ptr) & (alignment - 1);
			return misaligned != 0 ? (alignment - misaligned) : 0;
		}

		[[no_unique_address]] AllocatorRef<Allocator> m_allocator;
		U64 m_size = 0;
		U64 m_position = 0;
		Byte* m_buffer = nullptr;
	};
}
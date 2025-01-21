# RexCore
A collection of c++ utilities and data structures.

### Data structures `rexcore/containers/`
Natvis visualizations for the container types are in `rexcore/natvis/containers.natvis`.
- `Deque`, implemented with a list of fixed-size blocks.
- `Function` : skarupke_function.
- `Map` and `Set` : martinus's unordered_dense.
- `RingBuffer`
- `UniquePtr`, `SharedPtr` (not thread safe) and `AtomicSharedPtr` (thread safe).
- `Stack`, implemented as a list of blocks where each new block is twice the size of the last. Faster than MSVC's `std::stack` and `std::vector` for push_back and pop_back.
- `Vector`, `Span`
- `InplaceVector`, functionally equivalent to `Vector`, but with a starting buffer of a specified size allocated inplace.
- `FixedVector`, A fixed-size array that cannot resize.
- `String` and `WString`, sso enabled resizable string.
- `StringView` and `WStringView`, read-only string views
- `InplaceString` and `WInplaceString`, string with a specified sso size, equivalent of `InplaceVector` for strings.

### Allocators `rexcore/allocators.hpp`
`REX_CORE_TRACK_ALLOCS` can be defined to enable allocation tracking.
- `MallocAllocator`, uses malloc and free.
- `PageAllocator`, allocates in page increments.
- `ArenaAllocator`, reserves a fixed buffer and incrementally commits the needed pages as needed. The buffer can be very large (16GB by default) because the pages are reserved but not commited.
- `PoolAllocator`, pool allocator using an inplace free list for the available slots.

### Iterators `rexcore/iterators.hpp`
- `Zip`, iterate multiple containers at once, stops when one of the containers is at the end : `for (auto[a, b, c] : Iter::Zip(vecA, vecB, vecC))`
- `Enumerate`, iterate the values and indices at the same time : `for (auto[i, value] : Iter::Enumerate(vec))`
- `Skip`, skip the n first elements : `for (auto value : Iter::Skip(n, vec))`

### Time `rexcore/time.hpp`
- `StopWatch`, a stopwatch with the highest available resolution

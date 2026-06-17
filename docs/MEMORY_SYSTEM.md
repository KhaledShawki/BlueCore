# BlueMemory

BlueMemory is the memory management layer used by BlueCore. It provides the allocation infrastructure for the rest of the system.

## Responsibilities

BlueMemory handles the following areas:

- Allocator interface and invocation mechanism
- Multiple allocator types (heap, linear, pool)
- Allocation request structures
- Memory pool registry and descriptors
- Allocation tagging and categorization
- Runtime metrics and diagnostics
- Allocation tracking and leak detection
- Memory block abstractions
- Optional integration with external allocators (such as mimalloc)

## Allocator Model

BlueCore uses a lightweight, non-virtual allocator handle:

```cpp
struct Allocator
{
    void*        Context;
    AllocateFn   Allocate;
    ReallocateFn Reallocate;
    FreeFn       Free;
};
```

Allocator implementations are adapted using `AllocatorInvoker<TAllocator>`. This approach avoids virtual function overhead while keeping allocator implementations explicit and easily testable.

## Memory Blocks

BlueMemory defines two main block types:

- `MemoryBlock` — Represents an owned contiguous memory range.
- `VirtualMemoryBlock` — Represents memory that has been reserved and committed from the operating system.

These abstractions are used internally by allocators, tracking systems, and diagnostics.

## Backend Support

By default, BlueMemory uses the system allocator. An optional `mimalloc` backend can be enabled during project generation using:

```text
--memory-backend=mimalloc
```

When using mimalloc, the dependency must be provided externally. BlueCore does not redistribute mimalloc binaries, and public headers do not expose `mimalloc.h`.

## Further Reading

Detailed information about allocation rules, typed allocation, pool usage, and runtime allocation is available in:

- [Allocation Contract](ALLOCATION_CONTRACT.md)

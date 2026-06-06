# BlueMemory

BlueMemory is the allocation layer used by the Blue runtime modules.

## Responsibilities

- Allocator interface
- Heap, linear, and pool allocators
- Allocation request and free-request structures
- Pool registry and pool descriptors
- Allocation tags
- Runtime metrics
- Tracking and leak-detection hooks
- Memory blocks and virtual memory blocks
- Optional mimalloc backend integration

## Allocator model

Blue uses a small runtime allocator handle:

```cpp
struct Allocator
{
	void* Context;
	AllocateFn Allocate;
	ReallocateFn Reallocate;
	FreeFn Free;
};
```

Allocator implementations are adapted through `AllocatorInvoker<TAllocator>`. This avoids virtual functions and keeps allocator implementations explicit and testable.

## Blocks

`MemoryBlock` represents an owned memory range.

`VirtualMemoryBlock` represents OS-reserved and committed virtual memory.

These types are used by allocator implementations, tracking, diagnostics, and future profiling tools.

## mimalloc

mimalloc is hidden behind the BlueMemory backend layer. Public Blue headers do not include `mimalloc.h`.

Enable it during generation when the dependency is available:

```text
--memory-backend=mimalloc
```

The project does not redistribute mimalloc binaries.

## Allocation contract

Typed allocations, explicit pool allocation, runtime allocation metadata, and current exclusions are documented in `docs/ALLOCATION_CONTRACT.md`.

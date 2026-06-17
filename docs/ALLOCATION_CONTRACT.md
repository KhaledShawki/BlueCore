# Allocation Contract

This document defines the allocation rules used by BlueMemory.

## Core Principles

BlueMemory follows a strict allocation model with the following principles:

- All runtime allocations go through explicit request structures.
- Typed allocations determine their memory pool at compile time.
- Allocation diagnostics must not perform heap allocations.
- Out-of-memory reporting uses fixed-size storage.
- Global `operator new` and `operator delete` overrides are not supported.
- Array allocation and polymorphic deletion are not provided by the typed allocation interface.

## Typed Allocation Flow

Typed allocations follow this path:

```text
BlueNew<T>()
  -> MemoryNewInvoker<T>
  -> MemoryPoolResolver<T>
  -> TypedAllocationProxy<T, Pool>
  -> AllocatorProxy<AllocatorKind, Pool>
  -> backend
```

`BlueDelete<T>()` requires that `T` is exactly the type that was allocated. Mismatched delete calls are not supported.

## Pool Declaration

Memory pools are declared using the `BLUE_MEMORY_POOL` macro in `MemoryPools.def`:

```cpp
BLUE_MEMORY_POOL(Renderer, "Renderer", BLUE_MB(300), Default, ThreadCounters, true)
```

Each declaration generates:
- A corresponding `MemoryPoolId` enum value
- A `MemoryPoolPolicy<Pool>` specialization
- Default descriptors used by the pool registry

## Using Pools with Classes

Classes can declare which pool they should be allocated from:

```cpp
class RenderMesh
{
    BLUE_USE_MEMORY_POOL(Renderer)

public:
    RenderMesh() noexcept;
    ~RenderMesh() noexcept;
};
```

Usage:

```cpp
RenderMesh* mesh = Blue::BlueNew<RenderMesh>();
Blue::BlueDelete(mesh);
```

## Explicit Pool Allocation

Objects can be allocated into a specific pool at runtime:

```cpp
auto* object = Blue::BlueNewInPool<
    Blue::MemoryPoolId::Resources, ResourceObject
>();

Blue::BlueDeleteFromPool<
    Blue::MemoryPoolId::Resources
>(object);
```

An object allocated into an explicit pool must be freed using the matching pool-specific delete function.

## Runtime Allocation

For cases where typed allocation is not suitable, raw runtime allocation is available:

```cpp
Blue::AllocationRequest request = BLUE_POOL_ALLOCATION_REQUEST(
    byteSize,
    alignment,
    Blue::AllocationTag::ResourceBuffer,
    Blue::MemoryPoolId::Resources
);

void* data = Blue::BlueTryAllocate(request);

Blue::BlueFree(Blue::AllocationFreeRequest{
    data,
    byteSize,
    alignment,
    Blue::MemoryPoolId::Resources,
    Blue::AllocationTag::ResourceBuffer
});
```

When using runtime allocation, the exact size, alignment, pool, and tag must be provided on deallocation.

## Metrics and Diagnostics

BlueMemory tracks the following information per pool:

- Current and peak memory usage
- Total allocated and freed bytes
- Allocation and free counts
- Failed allocation counts
- Optional per-thread counters (depending on pool configuration)

## Current Limitations

The following features are intentionally not part of the current allocation contract:

- TLSF allocator
- Global `new`/`delete` overrides
- Array allocation support
- Polymorphic deletion helpers
- JSON export of allocation data
- Full allocation tracking tables

These may be considered in future revisions of the memory system.
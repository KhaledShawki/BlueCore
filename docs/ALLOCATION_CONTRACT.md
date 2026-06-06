# Allocation Contract

This document defines the current BlueMemory allocation contract used by runtime modules.

## Core rules

- Runtime allocation uses explicit request and free-request structures.
- Typed allocations resolve their memory pool at compile time.
- Allocation diagnostics must not allocate memory.
- Out-of-memory reporting uses fixed-capacity storage.
- Global `new` and `delete` overrides are not part of the current contract.
- Array allocation and polymorphic base-pointer deletion are intentionally not supported by the typed allocation helpers.

## Typed allocation flow

```text
BlueNew<T>()
  -> MemoryNewInvoker<T>
  -> MemoryPoolResolver<T>
  -> TypedAllocationProxy<T, Pool>
  -> AllocatorProxy<AllocatorKind, Pool>
  -> backend
```

`BlueDelete<T>()` requires `T` to be the exact allocated type.

## Pool declaration

Memory pools are declared in `MemoryPools.def`:

```cpp
BLUE_MEMORY_POOL(Renderer, "Renderer", BLUE_MB(300), Default, ThreadCounters, true)
```

The declaration generates:

- `MemoryPoolId`
- `MemoryPoolPolicy<Pool>`
- default pool descriptors used by the registry

## Class pool metadata

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
RenderMesh* mesh = Blue::BlueNew< RenderMesh >();
Blue::BlueDelete(mesh);
```

## Explicit pool allocation

```cpp
auto* object = Blue::BlueNewInPool< Blue::MemoryPoolId::Resources, ResourceObject >();
Blue::BlueDeleteFromPool< Blue::MemoryPoolId::Resources >(object);
```

An object allocated with an explicit pool must be released with the matching explicit-pool delete function.

## Runtime allocation

```cpp
Blue::AllocationRequest request = BLUE_POOL_ALLOCATION_REQUEST(
	byteSize,
	alignment,
	Blue::AllocationTag::ResourceBuffer,
	Blue::MemoryPoolId::Resources);

void* data = Blue::BlueTryAllocate(request);

Blue::BlueFree(Blue::AllocationFreeRequest{
	data,
	byteSize,
	alignment,
	Blue::MemoryPoolId::Resources,
	Blue::AllocationTag::ResourceBuffer});
```

Runtime free requires exact metadata: pointer, size, alignment, pool, and tag.

## Metrics

BlueMemory records pool usage, allocation counts, free counts, failure counts, peak usage, and optional per-thread counters depending on the pool configuration.

## Current exclusions

These features are outside the current allocation contract:

- TLSF allocator
- global allocation override
- array allocation helpers
- polymorphic deletion helpers
- JSON export
- full allocation tracking table

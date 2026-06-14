# BlueMemory Small Block Allocator

BlueMemory routes typed and runtime default allocations of 512 bytes or less through a small block allocator.

## Design goals

- Keep the typed allocation path predictable and branch-light.
- Reuse memory aggressively for latency-sensitive allocations.
- Avoid locks on the common allocation and free path.
- Avoid per-allocation heap metadata for small objects.
- Keep diagnostics measurable through pool metrics, thread metrics, and allocator stats.

## Architecture

The allocator owns fixed-size slabs. Each slab is split into one size class:

```text
16, 32, 64, 128, 256, 512 bytes
```

Each thread owns a small thread-local cache containing one free list per class. Allocation pops one block from the current thread cache. If the cache is empty, the allocator allocates one 64 KiB slab, splits it, and refills the cache.

Free pushes the block into the current thread cache for the matching size class. The block itself stores the next pointer while it is free, so there is no external per-block metadata.

## Routing

`AllocatorProxy<AllocatorKind::Default, Pool>` now routes:

```text
size <= 512 and alignment <= 512 and power-of-two alignment -> small block allocator
otherwise                                                   -> system backend
```

Pool metrics still account for the requested allocation size, not the internal size class. This keeps budgets meaningful and avoids exposing allocator implementation details to pool accounting.

## Shutdown

Slabs are released on `ShutdownMemorySystem()`. A generation counter invalidates old thread-local cache lists when the allocator is reinitialized.

## Performance implications

The hot path for a cached small allocation is:

```text
compute size class
pop from thread-local free list
update pool/thread counters according to policy
```

There is no backend allocation, no global lock, and no per-object header in that path.


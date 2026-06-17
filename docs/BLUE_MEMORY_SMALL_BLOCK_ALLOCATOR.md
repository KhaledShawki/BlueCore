# BlueMemory Small Block Allocator

BlueMemory routes typed and runtime default allocations of 512 bytes or less through a dedicated small block allocator.

## Design Goals

The small block allocator is designed to achieve the following:

- Keep the typed allocation path predictable and branch-light.
- Aggressively reuse memory for latency-sensitive allocations.
- Avoid locks on the common allocation and free paths.
- Avoid per-allocation heap metadata for small objects.
- Keep diagnostics measurable through pool metrics, thread metrics, and allocator statistics.

## Architecture

The allocator manages fixed-size slabs. Each slab is divided into blocks of a single size class:

```text
16, 32, 64, 128, 256, 512 bytes
```

Each thread maintains a small thread-local cache containing one free list per size class. 

- **Allocation**: Pops a block from the current thread’s cache for the appropriate size class. If the cache is empty, the allocator allocates a new 64 KiB slab, splits it into blocks, and refills the cache.
- **Free**: Pushes the block back into the current thread’s cache for the matching size class.

The freed block itself stores the next pointer (intrusive free list), so there is no external per-block metadata.

## Routing

`AllocatorProxy<AllocatorKind::Default, Pool>` routes allocations as follows:

```text
size <= 512 and alignment <= 512 and power-of-two alignment → small block allocator
otherwise                                                   → system backend
```

Pool metrics continue to track the originally requested size (not the internal size class). This preserves meaningful budget accounting and avoids leaking allocator implementation details.

## Shutdown

All slabs are released during `ShutdownMemorySystem()`. A generation counter is used to invalidate stale thread-local cache lists when the allocator is reinitialized.

## Performance Characteristics

The hot path for a cached small allocation consists of:

```text
compute size class
pop from thread-local free list
update pool/thread counters according to policy
```

This path involves no backend allocation, no global lock, and no per-object header.
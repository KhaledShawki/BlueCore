# Atomics

BlueSystem provides native atomic primitives without exposing `std::atomic` in the Blue abstraction layer.

## Backends

```text
MSVC       _Interlocked* intrinsics
GCC/Clang  __atomic_* builtins
```

The backend is selected by compiler/toolchain, not by operating system.

## Supported types

Supported atomic storage types include:

- `Bool`
- `Int32` / `Uint32`
- `Int64` / `Uint64`
- pointer types
- enums with 1-, 2-, 4-, or 8-byte underlying types
- `AtomicValue128` for aligned 16-byte snapshots

Fetch arithmetic is limited to 4-byte and 8-byte integral atomic storage types.

## Memory order

Blue exposes:

```cpp
enum class MemoryOrder : Uint8
{
	Relaxed,
	Acquire,
	Release,
	AcquireRelease,
	SequentiallyConsistent,
};
```

GCC/Clang maps these values to `__ATOMIC_*` constants. MSVC intrinsics may provide stronger ordering than requested; that is accepted for correctness.

## SpinLock

`SpinLock` is implemented with `AtomicUint32`:

```text
0 = unlocked
1 = locked
```

Use it only for tiny, low-contention critical sections. Do not use it around file I/O, blocking waits, long-running work, or platform calls that may stall.

# Processor

BlueSystem exposes a small processor information API for worker-thread configuration, diagnostics, metrics, and platform policy.

## Public API

```cpp
Blue::ProcessorArchitecture Blue::GetProcessorArchitecture() noexcept;
const Blue::Char* Blue::GetProcessorArchitectureName(Blue::ProcessorArchitecture architecture) noexcept;

Blue::Uint32 Blue::GetLogicalProcessorCount() noexcept;
Blue::Uint32 Blue::GetCacheLineSize() noexcept;
Blue::Uint32 Blue::GetRecommendedWorkerThreadCount(Blue::Uint32 reservedThreadCount = 1) noexcept;

Blue::ProcessorInfo Blue::QueryProcessorInfo() noexcept;
```

## Rules

- No heap allocation.
- No standard library containers.
- No platform headers in public Blue headers.
- Platform-specific queries stay in `src/Platform/<OS>/Processor_*.cpp`.
- Generic policy logic stays in `src/Processor.cpp`.

## Platform behavior

Windows uses `GetNativeSystemInfo` and `GetLogicalProcessorInformation`.

Linux uses `sysconf`.

macOS uses `sysctlbyname`.

Fallback cache line size is 64 bytes when the platform query is unavailable.

## Non-goals

This is not a full CPU feature detection layer. SIMD capabilities, NUMA topology, physical core count, cache hierarchy, and affinity masks should be handled by a separate topology layer when needed.

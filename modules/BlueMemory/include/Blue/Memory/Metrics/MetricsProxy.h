#pragma once

#include <Blue/Memory/MemoryMetricsMode.h>
#include <Blue/Memory/Metrics/MemoryThreadContext.h>

namespace Blue
{
template< MemoryMetricsMode Mode >
struct MetricsProxy
{
  static void RecordAllocate( MemoryPoolId, AllocatorKind, Size ) noexcept {}

  static void RecordFree( MemoryPoolId, AllocatorKind, Size ) noexcept {}

  static void RecordFailure( MemoryPoolId, AllocatorKind ) noexcept {}
};

template<>
struct MetricsProxy< MemoryMetricsMode::ThreadCounters >
{
  static void RecordAllocate( MemoryPoolId pool, AllocatorKind allocator, Size size ) noexcept
  {
    RecordThreadMemoryAllocation( pool, allocator, size );
  }

  static void RecordFree( MemoryPoolId pool, AllocatorKind allocator, Size size ) noexcept
  {
    RecordThreadMemoryFree( pool, allocator, size );
  }

  static void RecordFailure( MemoryPoolId pool, AllocatorKind allocator ) noexcept
  {
    RecordThreadMemoryFailure( pool, allocator );
  }
};

template<>
struct MetricsProxy< MemoryMetricsMode::FullDiagnostics > : MetricsProxy< MemoryMetricsMode::ThreadCounters >
{};
} // namespace Blue

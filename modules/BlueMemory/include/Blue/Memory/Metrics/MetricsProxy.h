#pragma once

#include <Blue/Memory/AllocatorKind.h>
#include <Blue/Memory/Config/BlueMemoryConfig.h>
#include <Blue/Memory/MemoryMetricsMode.h>
#include <Blue/Memory/Pool/MemoryPoolId.h>
#include <Blue/System/Types.h>

#if BLUE_MEMORY_ENABLE_METRICS
#  include <Blue/Memory/Metrics/MemoryThreadContext.h>
#endif

namespace Blue
{
template< MemoryMetricsMode Mode >
struct MetricsProxy
{
  static void RecordAllocate( MemoryPoolId, AllocatorKind, Size ) noexcept {}

  static void RecordFree( MemoryPoolId, AllocatorKind, Size ) noexcept {}

  static void RecordFailure( MemoryPoolId, AllocatorKind ) noexcept {}
};

#if BLUE_MEMORY_ENABLE_METRICS

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

#endif
} // namespace Blue

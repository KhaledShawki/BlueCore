#pragma once

#include <Blue/Memory/Allocation/AllocationValidation.h>
#include <Blue/Memory/AllocationFailurePolicy.h>
#include <Blue/Memory/AllocationFlags.h>
#include <Blue/Memory/Allocator/SmallBlockAllocator.h>
#include <Blue/Memory/Api.h>
#include <Blue/Memory/Backend/MemoryBackend.h>
#include <Blue/Memory/Config/BlueMemoryConfig.h>
#include <Blue/Memory/Metrics/MetricsProxy.h>
#include <Blue/Memory/Pool/MemoryPoolPolicy.h>
#include <Blue/Memory/Pool/MemoryPoolRegistry.h>
#include <Blue/Memory/Tracking/MemoryAllocationTracker.h>

namespace Blue
{
BLUE_MEMORY_API AllocationFailureInfo MakeAllocationFailureInfo( MemoryPoolId pool,
                                                                 AllocatorKind allocator,
                                                                 AllocationTag tag,
                                                                 Size size,
                                                                 Size alignment,
                                                                 AllocationFailureReason reason,
                                                                 SourceLocation location ) noexcept;

BLUE_MEMORY_API void ReportAllocationFailure( const AllocationFailureInfo& info ) noexcept;

BLUE_MEMORY_API void HandleAllocationFailure( const AllocationFailureInfo& info,
                                              AllocationFailurePolicy policy ) noexcept;

template< AllocatorKind Kind, MemoryPoolId Pool >
struct AllocatorProxy;

template< MemoryPoolId Pool >
struct AllocatorProxy< AllocatorKind::Default, Pool >
{
  static void* Allocate( Size size,
                         Size alignment,
                         AllocationTag tag,
                         SourceLocation location,
                         AllocationFlags flags = AllocationFlag_None ) noexcept
  {
#if BLUE_MEMORY_ENABLE_METRICS
    using Policy = MemoryPoolPolicy< Pool >;
    using Metrics = MetricsProxy< Policy::Metrics >;
#endif

#if !BLUE_MEMORY_ENABLE_TRACKING
    ( void ) flags;
#endif

    const Size normalizedAlignment = NormalizeAllocationAlignment( alignment );

#if BLUE_MEMORY_ENABLE_POOL_ACCOUNTING
    MemoryPoolRegistry& registry = GetMemoryPoolRegistry( );
    AllocationFailureReason reason = AllocationFailureReason::None;

    if ( !registry.TryReserve( Pool, size, reason ) )
    {
      registry.RecordFailure( Pool, reason );

#  if BLUE_MEMORY_ENABLE_METRICS
      Metrics::RecordFailure( Pool, AllocatorKind::Default );
#  endif

      ReportAllocationFailure(
        MakeAllocationFailureInfo( Pool, AllocatorKind::Default, tag, size, normalizedAlignment, reason, location ) );
      return nullptr;
    }
#endif

    void* pointer = IsSmallBlockAllocationSupported( size, normalizedAlignment )
                    ? AllocateSmallBlock( size, normalizedAlignment )
                    : MemoryBackend::Allocate( size, normalizedAlignment );
    if ( !pointer )
    {
#if BLUE_MEMORY_ENABLE_POOL_ACCOUNTING
      registry.CancelReservation( Pool, size );
      registry.RecordFailure( Pool, AllocationFailureReason::BackendFailure );
#endif

#if BLUE_MEMORY_ENABLE_METRICS
      Metrics::RecordFailure( Pool, AllocatorKind::Default );
#endif

      ReportAllocationFailure( MakeAllocationFailureInfo( Pool,
                                                          AllocatorKind::Default,
                                                          tag,
                                                          size,
                                                          normalizedAlignment,
                                                          AllocationFailureReason::BackendFailure,
                                                          location ) );
      return nullptr;
    }

#if BLUE_MEMORY_ENABLE_TRACKING
    if ( !HasAllocationFlag( flags, AllocationFlag_NoTracking ) )
    {
      TrackMemoryAllocation(
        MemoryAllocationRecord{ pointer, size, normalizedAlignment, Pool, tag, AllocatorKind::Default, location } );
    }
#endif

#if BLUE_MEMORY_ENABLE_POOL_ACCOUNTING
    registry.CommitAllocation( Pool, size );
#endif

#if BLUE_MEMORY_ENABLE_METRICS
    Metrics::RecordAllocate( Pool, AllocatorKind::Default, size );
#endif

    return pointer;
  }

  static void Free( void* pointer, Size size, Size alignment ) noexcept
  {
#if BLUE_MEMORY_ENABLE_METRICS
    using Policy = MemoryPoolPolicy< Pool >;
    using Metrics = MetricsProxy< Policy::Metrics >;
#endif

    if ( !pointer )
    {
      return;
    }

#if BLUE_MEMORY_ENABLE_TRACKING
    MemoryAllocationRecord tracked = { };
    if ( UntrackMemoryAllocation( pointer, tracked ) )
    {
      size = tracked.ByteSize;
      alignment = tracked.Alignment;
    }
#endif

    alignment = NormalizeAllocationAlignment( alignment );

    if ( IsSmallBlockAllocationSupported( size, alignment ) )
    {
      FreeSmallBlock( pointer, size, alignment );
    }
    else
    {
      MemoryBackend::Free( pointer, size, alignment );
    }

#if BLUE_MEMORY_ENABLE_POOL_ACCOUNTING
    GetMemoryPoolRegistry( ).RecordFree( Pool, size );
#endif

#if BLUE_MEMORY_ENABLE_METRICS
    Metrics::RecordFree( Pool, AllocatorKind::Default, size );
#endif
  }
};
} // namespace Blue

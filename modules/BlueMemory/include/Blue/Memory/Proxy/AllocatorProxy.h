#pragma once

#include <Blue/Memory/AllocationFailurePolicy.h>
#include <Blue/Memory/AllocationFlags.h>
#include <Blue/Memory/Allocator/SmallBlockAllocator.h>
#include <Blue/Memory/Api.h>
#include <Blue/Memory/Backend/SystemMemoryBackend.h>
#include <Blue/Memory/Metrics/MetricsProxy.h>
#include <Blue/Memory/Oom/OomReporter.h>
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
                         AllocationFlags flags,
                         SourceLocation location ) noexcept
  {
    using Policy = MemoryPoolPolicy< Pool >;
    using Metrics = MetricsProxy< Policy::Metrics >;

    MemoryPoolRegistry& registry = GetMemoryPoolRegistry( );
    AllocationFailureReason reason = AllocationFailureReason::None;

    if ( !registry.TryReserve( Pool, size, reason ) )
    {
      registry.RecordFailure( Pool, reason );
      Metrics::RecordFailure( Pool, AllocatorKind::Default );
      RecordOomReport(
        MakeAllocationFailureInfo( Pool, AllocatorKind::Default, tag, size, alignment, reason, location ) );
      return nullptr;
    }

    void* pointer = IsSmallBlockAllocationSupported( size, alignment )
                    ? AllocateSmallBlock( size, alignment )
                    : SystemMemoryBackend::Allocate( size, alignment );
    if ( !pointer )
    {
      registry.CancelReservation( Pool, size );
      registry.RecordFailure( Pool, AllocationFailureReason::BackendFailure );
      Metrics::RecordFailure( Pool, AllocatorKind::Default );
      RecordOomReport( MakeAllocationFailureInfo( Pool,
                                                  AllocatorKind::Default,
                                                  tag,
                                                  size,
                                                  alignment,
                                                  AllocationFailureReason::BackendFailure,
                                                  location ) );
      return nullptr;
    }

#if BLUE_ENABLE_MEMORY_TRACKING
    if ( !HasAllocationFlag( flags, AllocationFlag_NoTracking ) )
    {
      TrackMemoryAllocation(
        MemoryAllocationRecord{ pointer, size, alignment, Pool, tag, AllocatorKind::Default, location } );
    }
#endif

    registry.CommitAllocation( Pool, size );
    Metrics::RecordAllocate( Pool, AllocatorKind::Default, size );
    return pointer;
  }

  static void Free( void* pointer, Size size, Size alignment ) noexcept
  {
    using Policy = MemoryPoolPolicy< Pool >;
    using Metrics = MetricsProxy< Policy::Metrics >;

    if ( !pointer )
    {
      return;
    }

#if BLUE_ENABLE_MEMORY_TRACKING
    MemoryAllocationRecord tracked = { };
    if ( UntrackMemoryAllocation( pointer, tracked ) )
    {
      size = tracked.ByteSize;
      alignment = tracked.Alignment;
    }
#endif

    if ( IsSmallBlockAllocationSupported( size, alignment ) )
    {
      FreeSmallBlock( pointer, size, alignment );
    }
    else
    {
      SystemMemoryBackend::Free( pointer, size, alignment );
    }
    GetMemoryPoolRegistry( ).RecordFree( Pool, size );
    Metrics::RecordFree( Pool, AllocatorKind::Default, size );
  }
};
} // namespace Blue

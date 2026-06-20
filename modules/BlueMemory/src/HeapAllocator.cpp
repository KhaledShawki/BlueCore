// Copyright (c) Khaled Shawki. All rights reserved.

#include "Pch.h"

#include <Blue/Memory/Allocation/AllocationValidation.h>
#include <Blue/Memory/Backend/MemoryBackend.h>
#include <Blue/Memory/HeapAllocator.h>
#include <Blue/Memory/MemoryMetrics.h>
#include <Blue/Memory/Oom/OomReporter.h>
#include <Blue/Memory/Pool/MemoryPoolRegistry.h>
#include <Blue/Memory/Proxy/AllocatorProxy.h>
#include <Blue/Memory/Tracking/MemoryAllocationTracker.h>

#include <string.h>


namespace Blue
{

AllocationResult HeapAllocator::Allocate( const AllocationRequest& request )
{
  const AllocationRequest normalizedRequest = NormalizeAllocationRequest( request );

  MemoryPoolRegistry& registry = GetMemoryPoolRegistry( );
  AllocationFailureReason reason = AllocationFailureReason::None;
  Bool reserved = false;

  if ( registry.IsInitialized( ) )
  {
    reserved = registry.TryReserve( normalizedRequest.Pool, normalizedRequest.ByteSize, reason );
    if ( !reserved )
    {
      registry.RecordFailure( normalizedRequest.Pool, reason );
      RecordOomReport(
        MakeAllocationFailureInfo( normalizedRequest.Pool,
                                   AllocatorKind::Heap,
                                   normalizedRequest.Tag,
                                   normalizedRequest.ByteSize,
                                   normalizedRequest.Alignment,
                                   reason,
                                   { normalizedRequest.File, normalizedRequest.Function, normalizedRequest.Line } ) );
      return { nullptr, 0 };
    }
  }

  void* pointer = MemoryBackend::Allocate( normalizedRequest.ByteSize, normalizedRequest.Alignment );
  if ( !pointer )
  {
    if ( reserved )
    {
      registry.CancelReservation( normalizedRequest.Pool, normalizedRequest.ByteSize );
      registry.RecordFailure( normalizedRequest.Pool, AllocationFailureReason::BackendFailure );
    }
    return { nullptr, 0 };
  }

  if ( HasAllocationFlag( normalizedRequest.Flags, AllocationFlag_ZeroMemory ) )
  {
    memset( pointer, 0, normalizedRequest.ByteSize );
  }

  if ( reserved )
  {
    registry.CommitAllocation( normalizedRequest.Pool, normalizedRequest.ByteSize );
  }

#if BLUE_ENABLE_MEMORY_TRACKING
  if ( !HasAllocationFlag( normalizedRequest.Flags, AllocationFlag_NoTracking ) )
  {
    TrackMemoryAllocation( MemoryAllocationRecord{
      pointer,
      normalizedRequest.ByteSize,
      normalizedRequest.Alignment,
      normalizedRequest.Pool,
      normalizedRequest.Tag,
      AllocatorKind::Heap,
      { normalizedRequest.File, normalizedRequest.Function, normalizedRequest.Line }
    } );
  }
#endif

  RecordMemoryAllocation( normalizedRequest.ByteSize );
  return { pointer, normalizedRequest.ByteSize };
}
AllocationResult HeapAllocator::Reallocate( void* pointer, Size oldSize, const AllocationRequest& request )
{
#if BLUE_ENABLE_MEMORY_TRACKING
  MemoryAllocationRecord existingRecord = { };
  if ( TryFindTrackedMemoryAllocation( pointer, existingRecord ) && existingRecord.ByteSize != 0 )
  {
    oldSize = existingRecord.ByteSize;
  }
#endif

  MemoryPoolRegistry& registry = GetMemoryPoolRegistry( );
  AllocationFailureReason reason = AllocationFailureReason::None;
  const bool grows = request.ByteSize > oldSize;
  const Size delta = grows ? request.ByteSize - oldSize : oldSize - request.ByteSize;

  if ( grows && registry.IsInitialized( ) )
  {
    if ( !registry.TryReserve( request.Pool, delta, reason ) )
    {
      registry.RecordFailure( request.Pool, reason );
      RecordOomReport( MakeAllocationFailureInfo( request.Pool,
                                                  AllocatorKind::Heap,
                                                  request.Tag,
                                                  request.ByteSize,
                                                  request.Alignment,
                                                  reason,
                                                  { request.File, request.Function, request.Line } ) );
      return { nullptr, 0 };
    }
  }

  void* newPointer = MemoryBackend::Reallocate( pointer, oldSize, request.ByteSize, request.Alignment );
  if ( !newPointer )
  {
    if ( grows && registry.IsInitialized( ) )
    {
      registry.CancelReservation( request.Pool, delta );
      registry.RecordFailure( request.Pool, AllocationFailureReason::BackendFailure );
    }

    return { nullptr, 0 };
  }

  if ( HasAllocationFlag( request.Flags, AllocationFlag_ZeroMemory ) && request.ByteSize > oldSize )
  {
    Byte* newBytes = static_cast< Byte* >( newPointer );
    memset( newBytes + oldSize, 0, request.ByteSize - oldSize );
  }

  if ( registry.IsInitialized( ) && delta > 0 )
  {
    if ( grows )
    {
      registry.CommitAllocation( request.Pool, delta );
    }
    else
    {
      registry.RecordFree( request.Pool, delta );
    }
  }

#if BLUE_ENABLE_MEMORY_TRACKING
  MemoryAllocationRecord tracked = { };
  if ( UntrackMemoryAllocation( pointer, tracked ) && tracked.ByteSize != 0 )
  {
    oldSize = tracked.ByteSize;
  }

  if ( !HasAllocationFlag( request.Flags, AllocationFlag_NoTracking ) )
  {
    TrackMemoryAllocation( MemoryAllocationRecord{
      newPointer,
      request.ByteSize,
      request.Alignment,
      request.Pool,
      request.Tag,
      AllocatorKind::Heap,
      { request.File, request.Function, request.Line }
    } );
  }
#endif

  RecordMemoryReallocation( oldSize, request.ByteSize );
  return { newPointer, request.ByteSize };
}

void HeapAllocator::Free( const AllocationFreeRequest& request )
{
  static_cast< void >( request.Alignment );
  if ( !request.Pointer )
  {
    return;
  }

  AllocationFreeRequest effectiveRequest = request;
#if BLUE_ENABLE_MEMORY_TRACKING
  MemoryAllocationRecord tracked = { };
  if ( UntrackMemoryAllocation( request.Pointer, tracked ) )
  {
    effectiveRequest.ByteSize = tracked.ByteSize;
    effectiveRequest.Alignment = tracked.Alignment;
    effectiveRequest.Pool = tracked.Pool;
    effectiveRequest.Tag = tracked.Tag;
  }
#endif

  MemoryBackend::Free( effectiveRequest.Pointer, effectiveRequest.ByteSize, effectiveRequest.Alignment );
  if ( GetMemoryPoolRegistry( ).IsInitialized( ) )
  {
    GetMemoryPoolRegistry( ).RecordFree( effectiveRequest.Pool, effectiveRequest.ByteSize );
  }
  RecordMemoryFree( effectiveRequest.ByteSize );
}
} // namespace Blue

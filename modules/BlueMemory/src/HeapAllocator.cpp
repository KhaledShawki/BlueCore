#include <Blue/Memory/HeapAllocator.h>
#include <Blue/Memory/MemoryMetrics.h>
#include <Blue/Memory/Oom/OomReporter.h>
#include <Blue/Memory/Pool/MemoryPoolRegistry.h>
#include <Blue/Memory/Proxy/AllocatorProxy.h>

namespace Blue::Backend
{
void* BackendAllocate( Size size, Size alignment );
void* BackendReallocate( void* pointer, Size oldSize, Size newSize, Size alignment );
void BackendFree( void* pointer );
} // namespace Blue::Backend

namespace Blue
{
AllocationResult HeapAllocator::Allocate( const AllocationRequest& request )
{
  MemoryPoolRegistry& registry = GetMemoryPoolRegistry( );
  AllocationFailureReason reason = AllocationFailureReason::None;
  Bool reserved = false;

  if ( registry.IsInitialized( ) )
  {
    reserved = registry.TryReserve( request.Pool, request.ByteSize, reason );
    if ( !reserved )
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

  void* pointer = Backend::BackendAllocate( request.ByteSize, request.Alignment );
  if ( !pointer )
  {
    if ( reserved )
    {
      registry.CancelReservation( request.Pool, request.ByteSize );
      registry.RecordFailure( request.Pool, AllocationFailureReason::BackendFailure );
    }
    return { nullptr, 0 };
  }

  if ( reserved )
  {
    registry.CommitAllocation( request.Pool, request.ByteSize );
  }

  RecordMemoryAllocation( request.ByteSize );
  return { pointer, request.ByteSize };
}

AllocationResult HeapAllocator::Reallocate( void* pointer, Size oldSize, const AllocationRequest& request )
{
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

  void* newPointer = Backend::BackendReallocate( pointer, oldSize, request.ByteSize, request.Alignment );
  if ( !newPointer )
  {
    if ( grows && registry.IsInitialized( ) )
    {
      registry.CancelReservation( request.Pool, delta );
      registry.RecordFailure( request.Pool, AllocationFailureReason::BackendFailure );
    }

    return { nullptr, 0 };
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

  Backend::BackendFree( request.Pointer );
  if ( GetMemoryPoolRegistry( ).IsInitialized( ) )
  {
    GetMemoryPoolRegistry( ).RecordFree( request.Pool, request.ByteSize );
  }
  RecordMemoryFree( request.ByteSize );
}
} // namespace Blue

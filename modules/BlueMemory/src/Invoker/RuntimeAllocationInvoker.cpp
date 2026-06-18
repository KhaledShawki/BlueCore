#include <Blue/Memory/Invoker/RuntimeAllocationInvoker.h>
#include <Blue/Memory/Oom/OomReporter.h>
#include <Blue/Memory/Pool/MemoryPoolRegistry.h>
#include <Blue/System/Assert.h>

namespace Blue
{
void* RuntimeAllocationInvoker::TryAllocate( const AllocationRequest& request ) noexcept
{
  const AllocationValidationResult validation = ValidateAllocationRequest( request );
  if ( !validation.Valid )
  {
    RecordOomReport( MakeAllocationFailureInfo( request.Pool,
                                                AllocatorKind::Default,
                                                request.Tag,
                                                request.ByteSize,
                                                request.Alignment,
                                                validation.Reason,
                                                { request.File, request.Function, request.Line } ) );
    return nullptr;
  }

  return RuntimeAllocationProxy::Allocate( request );
}

void* RuntimeAllocationInvoker::Allocate( const AllocationRequest& request ) noexcept
{
  void* pointer = TryAllocate( request );
  if ( !pointer )
  {
    HandleAllocationFailure( MakeAllocationFailureInfo( request.Pool,
                                                        AllocatorKind::Default,
                                                        request.Tag,
                                                        request.ByteSize,
                                                        request.Alignment,
                                                        AllocationFailureReason::OutOfMemory,
                                                        { request.File, request.Function, request.Line } ),
                             request.FailurePolicy );
  }

  return pointer;
}

void RuntimeAllocationInvoker::Free( const AllocationFreeRequest& request ) noexcept
{
  RuntimeAllocationProxy::Free( request );
}

void* BlueTryAllocate( const AllocationRequest& request ) noexcept
{
  return RuntimeAllocationInvoker::TryAllocate( request );
}

void* BlueAllocate( const AllocationRequest& request ) noexcept
{
  return RuntimeAllocationInvoker::Allocate( request );
}

void BlueFree( const AllocationFreeRequest& request ) noexcept
{
  RuntimeAllocationInvoker::Free( request );
}

void BlueFree( void* pointer, Size size, Size alignment, MemoryPoolId pool ) noexcept
{
  BlueFree( AllocationFreeRequest{ pointer, size, alignment, pool, AllocationTag::Unknown } );
}
} // namespace Blue

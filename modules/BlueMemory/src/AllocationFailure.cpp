#include <Blue/Memory/MemorySystem.h>
#include <Blue/Memory/Oom/OomReporter.h>
#include <Blue/Memory/Pool/MemoryPoolRegistry.h>
#include <Blue/Memory/Proxy/AllocatorProxy.h>
#include <Blue/System/Assert.h>

namespace Blue
{
AllocationFailureInfo MakeAllocationFailureInfo( MemoryPoolId pool,
                                                 AllocatorKind allocator,
                                                 AllocationTag tag,
                                                 Size size,
                                                 Size alignment,
                                                 AllocationFailureReason reason,
                                                 SourceLocation location ) noexcept
{
  AllocationFailureInfo info = { };
  info.Reason = reason;
  info.Pool = pool;
  info.Allocator = allocator;
  info.Tag = tag;
  info.RequestedSize = size;
  info.RequestedAlignment = alignment;
  info.Location = location;

  MemoryPoolStats stats = { };
  if ( GetMemoryPoolRegistry( ).CaptureStats( pool, stats ) )
  {
    info.PoolBudgetBytes = stats.BudgetBytes;
    info.PoolCurrentBytes = static_cast< Size >( stats.CurrentBytes );
    info.PoolPeakBytes = static_cast< Size >( stats.PeakBytes );
  }

  return info;
}

void HandleAllocationFailure( const AllocationFailureInfo& info, AllocationFailurePolicy policy ) noexcept
{
  RecordOomReport( info );

  if ( policy == AllocationFailurePolicy::ReturnNull )
  {
    return;
  }

  AllocationFailureHandler handler = GetMemoryAllocationFailureHandler( );
  if ( handler )
  {
    handler( info );
  }

  BLUE_ASSERT( false && "BlueMemory allocation failed." );
}
} // namespace Blue

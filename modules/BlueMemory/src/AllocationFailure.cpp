// Copyright (c) Khaled Shawki. All rights reserved.

#include "Pch.h"

#include <Blue/Memory/Config/BlueMemoryConfig.h>
#include <Blue/Memory/MemorySystem.h>
#include <Blue/Memory/Oom/OomReporter.h>
#include <Blue/Memory/Pool/MemoryPoolRegistry.h>
#include <Blue/Memory/Proxy/AllocatorProxy.h>
#include <Blue/System/Assert.h>

#include <stdlib.h>

namespace Blue
{
namespace
{
#if BLUE_MEMORY_ENABLE_OOM_REPORTS
Bool ShouldRecordOomReport( OomPolicy policy ) noexcept
{
  return policy == OomPolicy::ReportAndReturnNull || policy == OomPolicy::Callback || policy == OomPolicy::Fatal;
}
#endif

Bool ShouldAbortAllocationFailure( AllocationFailurePolicy policy, OomPolicy runtimePolicy ) noexcept
{
  return policy == AllocationFailurePolicy::Abort || policy == AllocationFailurePolicy::CallHandlerThenAbort ||
         runtimePolicy == OomPolicy::Fatal;
}

Bool ShouldCallAllocationFailureHandler( AllocationFailurePolicy policy, OomPolicy runtimePolicy ) noexcept
{
  return policy == AllocationFailurePolicy::CallHandlerThenAbort || runtimePolicy == OomPolicy::Fatal;
}

void InvokeOomCallback( const AllocationFailureInfo& info, const BlueMemorySettings& settings ) noexcept
{
  if ( settings.OutOfMemoryPolicy != OomPolicy::Callback || !settings.OutOfMemoryCallback )
  {
    return;
  }

  settings.OutOfMemoryCallback( info.Reason, info.RequestedSize );
}

[[noreturn]] void AbortAllocationFailure( ) noexcept
{
  BLUE_ASSERT( false && "BlueMemory allocation failed." );
  abort( );
}
} // namespace

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

void ReportAllocationFailure( const AllocationFailureInfo& info ) noexcept
{
  const BlueMemorySettings& settings = GetMemorySettings( );

#if BLUE_MEMORY_ENABLE_OOM_REPORTS
  if ( ShouldRecordOomReport( settings.OutOfMemoryPolicy ) )
  {
    RecordOomReport( info );
  }
#endif

  InvokeOomCallback( info, settings );
}

void HandleAllocationFailure( const AllocationFailureInfo& info, AllocationFailurePolicy policy ) noexcept
{
  const BlueMemorySettings& settings = GetMemorySettings( );

  if ( ShouldCallAllocationFailureHandler( policy, settings.OutOfMemoryPolicy ) )
  {
    AllocationFailureHandler handler = GetMemoryAllocationFailureHandler( );
    if ( handler )
    {
      handler( info );
    }
  }

  if ( !ShouldAbortAllocationFailure( policy, settings.OutOfMemoryPolicy ) )
  {
    return;
  }

  AbortAllocationFailure( );
}
} // namespace Blue

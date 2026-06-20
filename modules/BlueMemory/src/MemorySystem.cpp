// Copyright (c) Khaled Shawki. All rights reserved.

#include "Pch.h"

#include <Blue/Memory/Allocator/SmallBlockAllocator.h>
#include <Blue/Memory/AllocatorInvoker.h>
#include <Blue/Memory/MemorySystem.h>
#include <Blue/Memory/Metrics/MemoryThreadContext.h>
#include <Blue/Memory/Oom/OomReporter.h>
#include <Blue/Memory/Pool/MemoryPoolRegistry.h>
#include <Blue/Memory/Tracking/MemoryAllocationTracker.h>
#include <Blue/System/Log/LogMacros.h>

#include <stdio.h>

#ifndef BLUE_MEMORY_ENABLE_LEAK_DETECTION
#  if defined( BLUE_SHIPPING )
#    define BLUE_MEMORY_ENABLE_LEAK_DETECTION 0
#  else
#    define BLUE_MEMORY_ENABLE_LEAK_DETECTION BLUE_ENABLE_LOGGING
#  endif
#endif

namespace Blue
{
BLUE_DEFINE_LOG_CATEGORY( LogMemory, LogLevel::Info );

namespace
{
struct MemorySystemState
{
  Bool Initialized = false;
  HeapAllocator Heap = { };
  Allocator DefaultAllocator = { };
  MemorySystemDesc Desc = { };
};

static MemorySystemState s_Memory = { };

#if BLUE_MEMORY_ENABLE_LEAK_DETECTION

constexpr Size MemoryLeakLogMessageCapacity = 256;

const Char* GetSafeMemoryPoolName( const MemoryPoolStats& stats ) noexcept
{
  return stats.Name ? stats.Name : "<Unknown>";
}

Bool CaptureMemoryPoolStatsByIndex( Size poolIndex, MemoryPoolStats& outStats ) noexcept
{
  if ( poolIndex >= MemoryPoolCount )
  {
    outStats = { };
    return false;
  }

  const MemoryPoolId pool = static_cast< MemoryPoolId >( poolIndex );
  return GetMemoryPoolRegistry( ).CaptureStats( pool, outStats );
}


Bool HasLiveRequestedBytes( const MemoryPoolStats& stats ) noexcept
{
  return stats.CurrentBytes != 0;
}

void ReportLiveMemoryPoolAllocation( const MemoryPoolStats& stats ) noexcept
{
  Char message[ MemoryLeakLogMessageCapacity ] = { };

  snprintf( message,
            sizeof( message ),
            "Memory pool leak detected: pool=%s live allocation bytes=%llu peak_bytes=%llu "
            "allocations=%llu frees=%llu total_allocated=%llu total_freed=%llu",
            GetSafeMemoryPoolName( stats ),
            static_cast< Uint64 >( stats.CurrentBytes ),
            static_cast< Uint64 >( stats.PeakBytes ),
            static_cast< Uint64 >( stats.AllocationCount ),
            static_cast< Uint64 >( stats.FreeCount ),
            static_cast< Uint64 >( stats.TotalAllocatedBytes ),
            static_cast< Uint64 >( stats.TotalFreedBytes ) );

  BLUE_LOG_ERROR( LogMemory, message );
}


Bool ReportLiveMemoryPoolAllocations( ) noexcept
{
  Bool leakDetected = false;

  for ( Size poolIndex = 0; poolIndex < MemoryPoolCount; ++poolIndex )
  {
    MemoryPoolStats stats = { };
    if ( !CaptureMemoryPoolStatsByIndex( poolIndex, stats ) )
    {
      continue;
    }

    if ( !HasLiveRequestedBytes( stats ) )
    {
      continue;
    }

    ReportLiveMemoryPoolAllocation( stats );
    leakDetected = true;
  }

  return leakDetected;
}

void RunShutdownLeakDetection( ) noexcept
{
  if ( !s_Memory.Desc.EnableLeakDetection )
  {
    return;
  }

#  if BLUE_ENABLE_MEMORY_TRACKING
  if ( IsMemoryAllocationTrackingEnabled( ) )
  {
    ReportLiveTrackedMemoryAllocations( );
    return;
  }
#  endif

  ReportLiveMemoryPoolAllocations( );
}
#else
void RunShutdownLeakDetection( ) noexcept {}
#endif
} // namespace

Result InitializeMemorySystem( const MemorySystemDesc& desc )
{
  if ( s_Memory.Initialized )
  {
    return Failure( ResultCode::AlreadyInitialized );
  }

  ResetMemoryMetrics( );
  ConfigureOomReporter( desc.OomReportBuffer, desc.OomReportCapacity );

  MemoryPoolRegistry& registry = GetMemoryPoolRegistry( );
  if ( !registry.Initialize( GetDefaultMemoryPoolDescs( ), GetDefaultMemoryPoolDescCount( ) ) )
  {
    return Failure( ResultCode::UnknownFailure );
  }

#if BLUE_ENABLE_MEMORY_TRACKING
  if ( desc.EnableTracking && !InitializeMemoryAllocationTracker( desc.TrackingCapacity ) )
  {
    GetMemoryPoolRegistry( ).Shutdown( );
    ClearOomReports( );
    return Failure( ResultCode::UnknownFailure );
  }
#endif

  RegisterMemoryThread( "Main" );
  if ( !InitializeSmallBlockAllocator( ) )
  {
    GetMemoryPoolRegistry( ).Shutdown( );
#if BLUE_ENABLE_MEMORY_TRACKING
    ShutdownMemoryAllocationTracker( );
#endif
    UnregisterMemoryThread( );
    return Failure( ResultCode::UnknownFailure );
  }

  s_Memory.Heap = HeapAllocator( );
  s_Memory.DefaultAllocator = AllocatorInvoker< HeapAllocator >::Make( s_Memory.Heap );
  s_Memory.Desc = desc;
  s_Memory.Initialized = true;

  BLUE_LOG_INFO( LogMemory, "BlueMemory initialized" );
  return Success( );
}

void ShutdownMemorySystem( )
{
  if ( !s_Memory.Initialized )
  {
    return;
  }

  RunShutdownLeakDetection( );

  ShutdownSmallBlockAllocator( );
#if BLUE_ENABLE_MEMORY_TRACKING
  ShutdownMemoryAllocationTracker( );
#endif
  GetMemoryPoolRegistry( ).Shutdown( );
  ClearOomReports( );
  UnregisterMemoryThread( );

  BLUE_LOG_INFO( LogMemory, "BlueMemory shutdown" );
  s_Memory = { };
}

bool IsMemorySystemInitialized( )
{
  return s_Memory.Initialized;
}

Allocator GetDefaultAllocator( )
{
  BLUE_ASSERT( s_Memory.Initialized );
  return s_Memory.DefaultAllocator;
}

AllocationFailureHandler GetMemoryAllocationFailureHandler( ) noexcept
{
  return s_Memory.Desc.FailureHandler;
}

Bool CaptureMemoryPoolStats( MemoryPoolId pool, MemoryPoolStats& outStats ) noexcept
{
  return GetMemoryPoolRegistry( ).CaptureStats( pool, outStats );
}
} // namespace Blue

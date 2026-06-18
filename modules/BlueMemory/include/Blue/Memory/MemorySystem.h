#pragma once

#include <Blue/Memory/AllocationFailureInfo.h>
#include <Blue/Memory/Allocator.h>
#include <Blue/Memory/Api.h>
#include <Blue/Memory/HeapAllocator.h>
#include <Blue/Memory/MemoryMetrics.h>
#include <Blue/Memory/MemoryMetricsMode.h>
#include <Blue/Memory/Oom/OomReport.h>
#include <Blue/Memory/Pool/MemoryPoolRegistry.h>
#include <Blue/System/Result.h>

namespace Blue
{
struct MemorySystemDesc
{
  bool EnableMetrics = true;
  bool EnableTracking = false;
  bool EnableLeakDetection = false;
  Size TrackingCapacity = 0;
  MemoryMetricsMode DefaultMetricsMode = MemoryMetricsMode::Counters;
  AllocationFailureHandler FailureHandler = nullptr;
  OomReport* OomReportBuffer = nullptr;
  Size OomReportCapacity = 0;
};

BLUE_MEMORY_API Result InitializeMemorySystem( const MemorySystemDesc& desc );
BLUE_MEMORY_API void ShutdownMemorySystem( );
BLUE_MEMORY_API bool IsMemorySystemInitialized( );
BLUE_MEMORY_API Allocator GetDefaultAllocator( );
BLUE_MEMORY_API AllocationFailureHandler GetMemoryAllocationFailureHandler( ) noexcept;
BLUE_MEMORY_API Bool CaptureMemoryPoolStats( MemoryPoolId pool, MemoryPoolStats& outStats ) noexcept;
} // namespace Blue

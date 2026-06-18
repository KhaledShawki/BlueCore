#pragma once

#include <Blue/Memory/Api.h>
#include <Blue/System/Types.h>

namespace Blue
{
struct MemoryMetricsSnapshot
{
  Uint64 TotalAllocatedBytes;
  Uint64 TotalFreedBytes;
  Uint64 CurrentLiveBytes;
  Uint64 PeakLiveBytes;
  Uint64 AllocationCount;
  Uint64 FreeCount;
  Uint64 ReallocationCount;
};

BLUE_MEMORY_API void ResetMemoryMetrics( );
BLUE_MEMORY_API void RecordMemoryAllocation( Size size );
BLUE_MEMORY_API void RecordMemoryFree( Size size );
BLUE_MEMORY_API void RecordMemoryReallocation( Size oldSize, Size newSize );
BLUE_MEMORY_API MemoryMetricsSnapshot GetMemoryMetricsSnapshot( );
} // namespace Blue

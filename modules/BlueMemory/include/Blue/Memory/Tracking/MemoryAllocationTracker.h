#pragma once

#include <Blue/Memory/Allocation/AllocationFreeRequest.h>
#include <Blue/Memory/AllocationFailureInfo.h>
#include <Blue/Memory/AllocatorKind.h>
#include <Blue/Memory/Api.h>
#include <Blue/System/SourceLocation.h>
#include <Blue/System/Types.h>

#ifndef BLUE_ENABLE_MEMORY_TRACKING
#  define BLUE_ENABLE_MEMORY_TRACKING 0
#endif

namespace Blue
{
struct MemoryAllocationRecord
{
  void* Pointer = nullptr;
  Size ByteSize = 0;
  Size Alignment = 0;
  MemoryPoolId Pool = MemoryPoolId::System;
  AllocationTag Tag = AllocationTag::Unknown;
  AllocatorKind Allocator = AllocatorKind::Default;
  SourceLocation Location = { };
};

struct MemoryAllocationTrackerStats
{
  Bool Enabled = false;
  Size Capacity = 0;
  Size ActiveCount = 0;
  Uint64 TotalTrackedCount = 0;
  Uint64 TotalUntrackedCount = 0;
  Uint64 FailedTrackCount = 0;
  Uint64 UnknownFreeCount = 0;
  Uint64 CollisionProbeCount = 0;
};

BLUE_MEMORY_API Bool IsMemoryAllocationTrackingCompiledIn( ) noexcept;
BLUE_MEMORY_API Bool InitializeMemoryAllocationTracker( Size capacity ) noexcept;
BLUE_MEMORY_API void ShutdownMemoryAllocationTracker( ) noexcept;
BLUE_MEMORY_API Bool IsMemoryAllocationTrackingEnabled( ) noexcept;

BLUE_MEMORY_API Bool TrackMemoryAllocation( const MemoryAllocationRecord& record ) noexcept;
BLUE_MEMORY_API Bool TryFindTrackedMemoryAllocation( void* pointer, MemoryAllocationRecord& outRecord ) noexcept;
BLUE_MEMORY_API Bool UntrackMemoryAllocation( void* pointer, MemoryAllocationRecord& outRecord ) noexcept;

BLUE_MEMORY_API Size CaptureLiveMemoryAllocations( MemoryAllocationRecord* outRecords, Size capacity ) noexcept;
BLUE_MEMORY_API MemoryAllocationTrackerStats GetMemoryAllocationTrackerStats( ) noexcept;
BLUE_MEMORY_API Bool ReportLiveTrackedMemoryAllocations( ) noexcept;
} // namespace Blue

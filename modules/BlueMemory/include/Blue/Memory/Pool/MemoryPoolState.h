#pragma once

#include <Blue/Memory/Pool/MemoryPoolDesc.h>
#include <Blue/System/Threading/Atomic.h>

namespace Blue
{
struct MemoryPoolState
{
  MemoryPoolDesc Desc = { };
  AtomicUint64 CurrentBytes = AtomicUint64( 0 );
  AtomicUint64 PeakBytes = AtomicUint64( 0 );
  AtomicUint64 TotalAllocatedBytes = AtomicUint64( 0 );
  AtomicUint64 TotalFreedBytes = AtomicUint64( 0 );
  AtomicUint64 AllocationCount = AtomicUint64( 0 );
  AtomicUint64 FreeCount = AtomicUint64( 0 );
  AtomicUint64 FailedAllocationCount = AtomicUint64( 0 );
  AtomicUint64 BudgetExceededCount = AtomicUint64( 0 );
};
} // namespace Blue

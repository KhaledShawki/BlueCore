#pragma once

#include <Blue/Memory/AllocatorKind.h>
#include <Blue/Memory/MemoryMetricsMode.h>
#include <Blue/Memory/Pool/MemoryPoolId.h>
#include <Blue/System/Types.h>

namespace Blue
{
struct MemoryPoolStats
{
	MemoryPoolId Pool = MemoryPoolId::System;
	AllocatorKind Allocator = AllocatorKind::Default;
	MemoryMetricsMode Metrics = MemoryMetricsMode::Counters;
	const Char* Name = nullptr;
	Size BudgetBytes = 0;
	Uint64 CurrentBytes = 0;
	Uint64 PeakBytes = 0;
	Uint64 TotalAllocatedBytes = 0;
	Uint64 TotalFreedBytes = 0;
	Uint64 AllocationCount = 0;
	Uint64 FreeCount = 0;
	Uint64 FailedAllocationCount = 0;
	Uint64 BudgetExceededCount = 0;
};
} // namespace Blue

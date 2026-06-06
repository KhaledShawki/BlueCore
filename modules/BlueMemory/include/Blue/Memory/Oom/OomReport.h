#pragma once

#include <Blue/Memory/AllocationFailureReason.h>
#include <Blue/Memory/AllocationTag.h>
#include <Blue/Memory/AllocatorKind.h>
#include <Blue/Memory/Pool/MemoryPoolId.h>
#include <Blue/System/SourceLocation.h>
#include <Blue/System/Threading/ThreadTypes.h>
#include <Blue/System/Types.h>

namespace Blue
{
struct OomReport
{
	Uint64 SequenceId = 0;
	Uint64 TimestampTicks = 0;
	ThreadId NativeThreadId = 0;
	Uint32 ThreadIndex = 0;
	const Char* ThreadName = nullptr;
	MemoryPoolId Pool = MemoryPoolId::System;
	AllocatorKind Allocator = AllocatorKind::Default;
	AllocationTag Tag = AllocationTag::Unknown;
	Size RequestedSize = 0;
	Size RequestedAlignment = 0;
	Size PoolBudgetBytes = 0;
	Size PoolCurrentBytes = 0;
	Size PoolPeakBytes = 0;
	AllocationFailureReason Reason = AllocationFailureReason::None;
	SourceLocation Location = { };
};
} // namespace Blue

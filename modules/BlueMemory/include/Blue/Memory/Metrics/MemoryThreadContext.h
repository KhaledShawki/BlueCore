#pragma once

#include <Blue/Memory/AllocatorKind.h>
#include <Blue/Memory/Api.h>
#include <Blue/Memory/Pool/MemoryPoolId.h>
#include <Blue/System/Threading/ThreadTypes.h>
#include <Blue/System/Types.h>

namespace Blue
{
constexpr Size BlueMemoryMaxTrackedThreads = 128;

struct ThreadAllocatorMetrics
{
	MemoryPoolId Pool = MemoryPoolId::System;
	AllocatorKind Allocator = AllocatorKind::Default;
	Uint64 AllocationCount = 0;
	Uint64 FreeCount = 0;
	Uint64 FailedAllocationCount = 0;
	Uint64 CurrentBytes = 0;
	Uint64 PeakBytes = 0;
};

struct MemoryThreadContext
{
	Uint32 ThreadIndex = 0;
	ThreadId NativeThreadId = 0;
	const Char* ThreadName = nullptr;
	ThreadAllocatorMetrics PoolMetrics[ MemoryPoolCount ] = { };
	Bool Active = false;
};

BLUE_MEMORY_API Bool RegisterMemoryThread( const Char* name ) noexcept;
BLUE_MEMORY_API void UnregisterMemoryThread( ) noexcept;
BLUE_MEMORY_API MemoryThreadContext* GetCurrentMemoryThreadContext( ) noexcept;
BLUE_MEMORY_API void RecordThreadMemoryAllocation( MemoryPoolId pool, AllocatorKind allocator, Size size ) noexcept;
BLUE_MEMORY_API void RecordThreadMemoryFree( MemoryPoolId pool, AllocatorKind allocator, Size size ) noexcept;
BLUE_MEMORY_API void RecordThreadMemoryFailure( MemoryPoolId pool, AllocatorKind allocator ) noexcept;
BLUE_MEMORY_API Size CaptureMemoryThreadContexts( MemoryThreadContext* output, Size capacity ) noexcept;
} // namespace Blue

#include <Blue/Memory/Metrics/MemoryThreadContext.h>
#include <Blue/System/Threading/Atomic.h>
#include <Blue/System/Threading/Thread.h>

namespace Blue
{
namespace
{
MemoryThreadContext s_Contexts[ BlueMemoryMaxTrackedThreads ] = { };
AtomicUint32 s_NextThreadIndex( 0 );
thread_local MemoryThreadContext* t_Context = nullptr;

MemoryThreadContext* AcquireThreadContext( const Char* name ) noexcept
{
	if ( t_Context )
	{
		return t_Context;
	}

	const Uint32 index = s_NextThreadIndex.FetchAdd( 1, MemoryOrder::Relaxed );
	if ( index >= BlueMemoryMaxTrackedThreads )
	{
		return nullptr;
	}

	MemoryThreadContext& context = s_Contexts[ index ];
	context.ThreadIndex = index;
	context.NativeThreadId = GetCurrentThreadId( );
	context.ThreadName = name ? name : "Unregistered";
	context.Active = true;

	for ( Size poolIndex = 0; poolIndex < MemoryPoolCount; ++poolIndex )
	{
		context.PoolMetrics[ poolIndex ] = { };
		context.PoolMetrics[ poolIndex ].Pool = static_cast< MemoryPoolId >( poolIndex );
	}

	t_Context = &context;
	return t_Context;
}
} // namespace

Bool RegisterMemoryThread( const Char* name ) noexcept
{
	return AcquireThreadContext( name ) != nullptr;
}

void UnregisterMemoryThread( ) noexcept
{
	if ( t_Context )
	{
		t_Context->Active = false;
		t_Context = nullptr;
	}
}

MemoryThreadContext* GetCurrentMemoryThreadContext( ) noexcept
{
	return AcquireThreadContext( "Unregistered" );
}

void RecordThreadMemoryAllocation( MemoryPoolId pool, AllocatorKind allocator, Size size ) noexcept
{
	MemoryThreadContext* context = GetCurrentMemoryThreadContext( );
	if ( !context || !IsValidMemoryPoolId( pool ) )
	{
		return;
	}

	ThreadAllocatorMetrics& metrics = context->PoolMetrics[ ToMemoryPoolIndex( pool ) ];
	metrics.Pool = pool;
	metrics.Allocator = allocator;
	metrics.CurrentBytes += size;
	metrics.AllocationCount += 1;
	if ( metrics.CurrentBytes > metrics.PeakBytes )
	{
		metrics.PeakBytes = metrics.CurrentBytes;
	}
}

void RecordThreadMemoryFree( MemoryPoolId pool, AllocatorKind allocator, Size size ) noexcept
{
	MemoryThreadContext* context = GetCurrentMemoryThreadContext( );
	if ( !context || !IsValidMemoryPoolId( pool ) )
	{
		return;
	}

	ThreadAllocatorMetrics& metrics = context->PoolMetrics[ ToMemoryPoolIndex( pool ) ];
	metrics.Pool = pool;
	metrics.Allocator = allocator;
	metrics.FreeCount += 1;
	metrics.CurrentBytes = metrics.CurrentBytes >= size ? metrics.CurrentBytes - size : 0;
}

void RecordThreadMemoryFailure( MemoryPoolId pool, AllocatorKind allocator ) noexcept
{
	MemoryThreadContext* context = GetCurrentMemoryThreadContext( );
	if ( !context || !IsValidMemoryPoolId( pool ) )
	{
		return;
	}

	ThreadAllocatorMetrics& metrics = context->PoolMetrics[ ToMemoryPoolIndex( pool ) ];
	metrics.Pool = pool;
	metrics.Allocator = allocator;
	metrics.FailedAllocationCount += 1;
}

Size CaptureMemoryThreadContexts( MemoryThreadContext* output, Size capacity ) noexcept
{
	if ( !output || capacity == 0 )
	{
		return 0;
	}

	const Uint32 count = s_NextThreadIndex.Load( MemoryOrder::Acquire );
	const Size copyCount = count < capacity ? static_cast< Size >( count ) : capacity;
	for ( Size index = 0; index < copyCount; ++index )
	{
		output[ index ] = s_Contexts[ index ];
	}

	return copyCount;
}
} // namespace Blue

#include <Blue/Memory/Allocator/SmallBlockAllocator.h>
#include <Blue/Memory/AllocatorInvoker.h>
#include <Blue/Memory/MemorySystem.h>
#include <Blue/Memory/Metrics/MemoryThreadContext.h>
#include <Blue/Memory/Oom/OomReporter.h>
#include <Blue/Memory/Pool/MemoryPoolRegistry.h>
#include <Blue/System/Log/LogMacros.h>

namespace Blue
{
BLUE_DEFINE_LOG_CATEGORY( LogMemory, LogLevel::Info );

struct MemorySystemState
{
	bool Initialized = false;
	HeapAllocator Heap = { };
	Allocator DefaultAllocator = { };
	MemorySystemDesc Desc = { };
};

static MemorySystemState s_Memory = { };

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

	RegisterMemoryThread( "Main" );
	if ( !InitializeSmallBlockAllocator( ) )
	{
		GetMemoryPoolRegistry( ).Shutdown( );
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

	MemoryMetricsSnapshot metrics = GetMemoryMetricsSnapshot( );
	if ( s_Memory.Desc.EnableLeakDetection && metrics.CurrentLiveBytes != 0 )
	{
		BLUE_LOG_ERROR( LogMemory, "BlueMemory shutdown detected live allocations" );
	}

	ShutdownSmallBlockAllocator( );
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

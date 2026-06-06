#pragma once

#include <Blue/Memory/AllocationFailureReason.h>
#include <Blue/Memory/AllocationTag.h>
#include <Blue/Memory/AllocatorKind.h>
#include <Blue/Memory/Api.h>
#include <Blue/Memory/Pool/MemoryPoolState.h>
#include <Blue/Memory/Pool/MemoryPoolStats.h>
#include <Blue/System/SourceLocation.h>
#include <Blue/System/Types.h>

namespace Blue
{
class BLUE_MEMORY_API MemoryPoolRegistry
{
public:
	Bool Initialize( const MemoryPoolDesc* descs, Size count ) noexcept;
	void Shutdown( ) noexcept;
	Bool IsInitialized( ) const noexcept;

	MemoryPoolState* GetState( MemoryPoolId pool ) noexcept;
	const MemoryPoolState* GetState( MemoryPoolId pool ) const noexcept;

	Bool TryReserve( MemoryPoolId pool, Size size, AllocationFailureReason& outReason ) noexcept;
	void CommitAllocation( MemoryPoolId pool, Size size ) noexcept;
	void CancelReservation( MemoryPoolId pool, Size size ) noexcept;
	void RecordFree( MemoryPoolId pool, Size size ) noexcept;
	void RecordFailure( MemoryPoolId pool, AllocationFailureReason reason ) noexcept;

	Bool CaptureStats( MemoryPoolId pool, MemoryPoolStats& outStats ) const noexcept;

private:
	MemoryPoolState m_Pools[ MemoryPoolCount ] = { };
	Bool m_Initialized = false;
};

BLUE_MEMORY_API MemoryPoolRegistry& GetMemoryPoolRegistry( ) noexcept;
BLUE_MEMORY_API const MemoryPoolDesc* GetDefaultMemoryPoolDescs( ) noexcept;
BLUE_MEMORY_API Size GetDefaultMemoryPoolDescCount( ) noexcept;
} // namespace Blue

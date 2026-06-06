#pragma once

#include <Blue/Memory/AllocationFailurePolicy.h>
#include <Blue/Memory/Api.h>
#include <Blue/Memory/Backend/SystemMemoryBackend.h>
#include <Blue/Memory/Metrics/MetricsProxy.h>
#include <Blue/Memory/Oom/OomReporter.h>
#include <Blue/Memory/Pool/MemoryPoolPolicy.h>
#include <Blue/Memory/Pool/MemoryPoolRegistry.h>

namespace Blue
{
BLUE_MEMORY_API AllocationFailureInfo MakeAllocationFailureInfo( MemoryPoolId pool,
                                                                 AllocatorKind allocator,
                                                                 AllocationTag tag,
                                                                 Size size,
                                                                 Size alignment,
                                                                 AllocationFailureReason reason,
                                                                 SourceLocation location ) noexcept;

BLUE_MEMORY_API void HandleAllocationFailure( const AllocationFailureInfo& info,
                                              AllocationFailurePolicy policy ) noexcept;

template< AllocatorKind Kind, MemoryPoolId Pool >
struct AllocatorProxy;

template< MemoryPoolId Pool >
struct AllocatorProxy< AllocatorKind::Default, Pool >
{
	static void* Allocate( Size size, Size alignment, AllocationTag tag, SourceLocation location ) noexcept
	{
		using Policy = MemoryPoolPolicy< Pool >;
		using Metrics = MetricsProxy< Policy::Metrics >;

		MemoryPoolRegistry& registry = GetMemoryPoolRegistry( );
		AllocationFailureReason reason = AllocationFailureReason::None;

		if ( !registry.TryReserve( Pool, size, reason ) )
		{
			registry.RecordFailure( Pool, reason );
			Metrics::RecordFailure( Pool, AllocatorKind::Default );
			RecordOomReport(
			    MakeAllocationFailureInfo( Pool, AllocatorKind::Default, tag, size, alignment, reason, location ) );
			return nullptr;
		}

		void* pointer = SystemMemoryBackend::Allocate( size, alignment );
		if ( !pointer )
		{
			registry.CancelReservation( Pool, size );
			registry.RecordFailure( Pool, AllocationFailureReason::BackendFailure );
			Metrics::RecordFailure( Pool, AllocatorKind::Default );
			RecordOomReport( MakeAllocationFailureInfo( Pool,
			                                            AllocatorKind::Default,
			                                            tag,
			                                            size,
			                                            alignment,
			                                            AllocationFailureReason::BackendFailure,
			                                            location ) );
			return nullptr;
		}

		registry.CommitAllocation( Pool, size );
		Metrics::RecordAllocate( Pool, AllocatorKind::Default, size );
		return pointer;
	}

	static void Free( void* pointer, Size size, Size alignment ) noexcept
	{
		using Policy = MemoryPoolPolicy< Pool >;
		using Metrics = MetricsProxy< Policy::Metrics >;

		if ( !pointer )
		{
			return;
		}

		SystemMemoryBackend::Free( pointer, size, alignment );
		GetMemoryPoolRegistry( ).RecordFree( Pool, size );
		Metrics::RecordFree( Pool, AllocatorKind::Default, size );
	}
};
} // namespace Blue

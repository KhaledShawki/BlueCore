#include <Blue/Memory/Pool/MemoryPoolPolicy.h>
#include <Blue/Memory/Pool/MemoryPoolRegistry.h>

namespace Blue
{
namespace
{
MemoryPoolRegistry s_Registry;

constexpr MemoryPoolDesc s_DefaultPoolDescs[] = {
#define BLUE_MEMORY_POOL( Id, NameText, BudgetValue, AllocatorToken, MetricsToken, OomValue )                          \
	MemoryPoolPolicy< MemoryPoolId::Id >::GetDesc( ),
#include <Blue/Memory/Pool/MemoryPools.def>
#undef BLUE_MEMORY_POOL
};

void ResetState( MemoryPoolState& state, const MemoryPoolDesc& desc ) noexcept
{
	state.Desc = desc;
	state.CurrentBytes.Store( 0 );
	state.PeakBytes.Store( 0 );
	state.TotalAllocatedBytes.Store( 0 );
	state.TotalFreedBytes.Store( 0 );
	state.AllocationCount.Store( 0 );
	state.FreeCount.Store( 0 );
	state.FailedAllocationCount.Store( 0 );
	state.BudgetExceededCount.Store( 0 );
}

void UpdatePeak( AtomicUint64& peakCounter, Uint64 value ) noexcept
{
	Uint64 peak = peakCounter.Load( MemoryOrder::Relaxed );
	while ( value > peak && !peakCounter.CompareExchange( peak, value, MemoryOrder::AcquireRelease ) )
	{
	}
}
} // namespace

MemoryPoolRegistry& GetMemoryPoolRegistry( ) noexcept
{
	return s_Registry;
}

const MemoryPoolDesc* GetDefaultMemoryPoolDescs( ) noexcept
{
	return s_DefaultPoolDescs;
}

Size GetDefaultMemoryPoolDescCount( ) noexcept
{
	return sizeof( s_DefaultPoolDescs ) / sizeof( s_DefaultPoolDescs[ 0 ] );
}

Bool MemoryPoolRegistry::Initialize( const MemoryPoolDesc* descs, Size count ) noexcept
{
	if ( m_Initialized || !descs || count != MemoryPoolCount )
	{
		return false;
	}

	for ( Size index = 0; index < MemoryPoolCount; ++index )
	{
		if ( ToMemoryPoolIndex( descs[ index ].Id ) != index )
		{
			return false;
		}
	}

	for ( Size index = 0; index < MemoryPoolCount; ++index )
	{
		ResetState( m_Pools[ index ], descs[ index ] );
	}

	m_Initialized = true;
	return true;
}

void MemoryPoolRegistry::Shutdown( ) noexcept
{
	for ( Size index = 0; index < MemoryPoolCount; ++index )
	{
		ResetState( m_Pools[ index ], MemoryPoolDesc{ } );
	}

	m_Initialized = false;
}

Bool MemoryPoolRegistry::IsInitialized( ) const noexcept
{
	return m_Initialized;
}

MemoryPoolState* MemoryPoolRegistry::GetState( MemoryPoolId pool ) noexcept
{
	if ( !m_Initialized || !IsValidMemoryPoolId( pool ) )
	{
		return nullptr;
	}

	return &m_Pools[ ToMemoryPoolIndex( pool ) ];
}

const MemoryPoolState* MemoryPoolRegistry::GetState( MemoryPoolId pool ) const noexcept
{
	if ( !m_Initialized || !IsValidMemoryPoolId( pool ) )
	{
		return nullptr;
	}

	return &m_Pools[ ToMemoryPoolIndex( pool ) ];
}

Bool MemoryPoolRegistry::TryReserve( MemoryPoolId pool, Size size, AllocationFailureReason& outReason ) noexcept
{
	outReason = AllocationFailureReason::None;

	MemoryPoolState* state = GetState( pool );
	if ( !state )
	{
		outReason =
		    m_Initialized ? AllocationFailureReason::InvalidPool : AllocationFailureReason::SystemNotInitialized;
		return false;
	}

	if ( size == 0 )
	{
		outReason = AllocationFailureReason::InvalidSize;
		return false;
	}

	const Size budget = state->Desc.BudgetBytes;
	Uint64 current = state->CurrentBytes.Load( MemoryOrder::Relaxed );

	for ( ;; )
	{
		const Uint64 next = current + static_cast< Uint64 >( size );
		if ( next < current )
		{
			outReason = AllocationFailureReason::OutOfMemory;
			return false;
		}

		if ( budget != 0 && next > static_cast< Uint64 >( budget ) )
		{
			state->BudgetExceededCount.FetchAdd( 1, MemoryOrder::Relaxed );
			outReason = AllocationFailureReason::PoolBudgetExceeded;
			return false;
		}

		if ( state->CurrentBytes.CompareExchange( current, next, MemoryOrder::AcquireRelease ) )
		{
			return true;
		}
	}
}

void MemoryPoolRegistry::CommitAllocation( MemoryPoolId pool, Size size ) noexcept
{
	MemoryPoolState* state = GetState( pool );
	if ( !state )
	{
		return;
	}

	const Uint64 current = state->CurrentBytes.Load( MemoryOrder::Relaxed );
	state->TotalAllocatedBytes.FetchAdd( size, MemoryOrder::Relaxed );
	state->AllocationCount.FetchAdd( 1, MemoryOrder::Relaxed );
	UpdatePeak( state->PeakBytes, current );
}

void MemoryPoolRegistry::CancelReservation( MemoryPoolId pool, Size size ) noexcept
{
	MemoryPoolState* state = GetState( pool );
	if ( !state )
	{
		return;
	}

	state->CurrentBytes.FetchSub( size, MemoryOrder::Relaxed );
}

void MemoryPoolRegistry::RecordFree( MemoryPoolId pool, Size size ) noexcept
{
	MemoryPoolState* state = GetState( pool );
	if ( !state )
	{
		return;
	}

	state->CurrentBytes.FetchSub( size, MemoryOrder::Relaxed );
	state->TotalFreedBytes.FetchAdd( size, MemoryOrder::Relaxed );
	state->FreeCount.FetchAdd( 1, MemoryOrder::Relaxed );
}

void MemoryPoolRegistry::RecordFailure( MemoryPoolId pool, AllocationFailureReason ) noexcept
{
	MemoryPoolState* state = GetState( pool );
	if ( state )
	{
		state->FailedAllocationCount.FetchAdd( 1, MemoryOrder::Relaxed );
	}
}

Bool MemoryPoolRegistry::CaptureStats( MemoryPoolId pool, MemoryPoolStats& outStats ) const noexcept
{
	const MemoryPoolState* state = GetState( pool );
	if ( !state )
	{
		outStats = { };
		return false;
	}

	outStats.Pool = state->Desc.Id;
	outStats.Allocator = state->Desc.Allocator;
	outStats.Metrics = state->Desc.Metrics;
	outStats.Name = state->Desc.Name;
	outStats.BudgetBytes = state->Desc.BudgetBytes;
	outStats.CurrentBytes = state->CurrentBytes.Load( MemoryOrder::Relaxed );
	outStats.PeakBytes = state->PeakBytes.Load( MemoryOrder::Relaxed );
	outStats.TotalAllocatedBytes = state->TotalAllocatedBytes.Load( MemoryOrder::Relaxed );
	outStats.TotalFreedBytes = state->TotalFreedBytes.Load( MemoryOrder::Relaxed );
	outStats.AllocationCount = state->AllocationCount.Load( MemoryOrder::Relaxed );
	outStats.FreeCount = state->FreeCount.Load( MemoryOrder::Relaxed );
	outStats.FailedAllocationCount = state->FailedAllocationCount.Load( MemoryOrder::Relaxed );
	outStats.BudgetExceededCount = state->BudgetExceededCount.Load( MemoryOrder::Relaxed );
	return true;
}
} // namespace Blue

#include <Blue/Memory/MemoryMetrics.h>

#include <atomic>

namespace Blue
{
struct MemoryMetricsState
{
	std::atomic< Uint64 > TotalAllocatedBytes;
	std::atomic< Uint64 > TotalFreedBytes;
	std::atomic< Uint64 > CurrentLiveBytes;
	std::atomic< Uint64 > PeakLiveBytes;
	std::atomic< Uint64 > AllocationCount;
	std::atomic< Uint64 > FreeCount;
	std::atomic< Uint64 > ReallocationCount;
};

static MemoryMetricsState s_Metrics = { };

void ResetMemoryMetrics( )
{
	s_Metrics.TotalAllocatedBytes.store( 0 );
	s_Metrics.TotalFreedBytes.store( 0 );
	s_Metrics.CurrentLiveBytes.store( 0 );
	s_Metrics.PeakLiveBytes.store( 0 );
	s_Metrics.AllocationCount.store( 0 );
	s_Metrics.FreeCount.store( 0 );
	s_Metrics.ReallocationCount.store( 0 );
}

static void UpdatePeak( Uint64 current )
{
	Uint64 peak = s_Metrics.PeakLiveBytes.load( );
	while ( current > peak && !s_Metrics.PeakLiveBytes.compare_exchange_weak( peak, current ) )
	{
	}
}

void RecordMemoryAllocation( Size size )
{
	s_Metrics.TotalAllocatedBytes.fetch_add( size );
	const Uint64 current = s_Metrics.CurrentLiveBytes.fetch_add( size ) + size;
	s_Metrics.AllocationCount.fetch_add( 1 );
	UpdatePeak( current );
}

void RecordMemoryFree( Size size )
{
	s_Metrics.TotalFreedBytes.fetch_add( size );
	s_Metrics.CurrentLiveBytes.fetch_sub( size );
	s_Metrics.FreeCount.fetch_add( 1 );
}

void RecordMemoryReallocation( Size oldSize, Size newSize )
{
	if ( newSize > oldSize )
	{
		RecordMemoryAllocation( newSize - oldSize );
	}
	else if ( oldSize > newSize )
	{
		RecordMemoryFree( oldSize - newSize );
	}
	s_Metrics.ReallocationCount.fetch_add( 1 );
}

MemoryMetricsSnapshot GetMemoryMetricsSnapshot( )
{
	MemoryMetricsSnapshot snapshot = { };
	snapshot.TotalAllocatedBytes = s_Metrics.TotalAllocatedBytes.load( );
	snapshot.TotalFreedBytes = s_Metrics.TotalFreedBytes.load( );
	snapshot.CurrentLiveBytes = s_Metrics.CurrentLiveBytes.load( );
	snapshot.PeakLiveBytes = s_Metrics.PeakLiveBytes.load( );
	snapshot.AllocationCount = s_Metrics.AllocationCount.load( );
	snapshot.FreeCount = s_Metrics.FreeCount.load( );
	snapshot.ReallocationCount = s_Metrics.ReallocationCount.load( );
	return snapshot;
}
} // namespace Blue

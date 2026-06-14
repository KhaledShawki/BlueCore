#include <Blue/Memory/MemoryMetrics.h>
#include <Blue/System/Threading/Atomic.h>

namespace Blue
{
struct MemoryMetricsState
{
	AtomicUint64 TotalAllocatedBytes = AtomicUint64( 0 );
	AtomicUint64 TotalFreedBytes = AtomicUint64( 0 );
	AtomicUint64 CurrentLiveBytes = AtomicUint64( 0 );
	AtomicUint64 PeakLiveBytes = AtomicUint64( 0 );
	AtomicUint64 AllocationCount = AtomicUint64( 0 );
	AtomicUint64 FreeCount = AtomicUint64( 0 );
	AtomicUint64 ReallocationCount = AtomicUint64( 0 );
};

static MemoryMetricsState s_Metrics = { };

static void UpdatePeak( AtomicUint64& peakCounter, Uint64 value ) noexcept
{
	Uint64 peak = peakCounter.Load( MemoryOrder::Relaxed );
	while ( value > peak && !peakCounter.CompareExchange( peak, value, MemoryOrder::AcquireRelease ) )
	{
	}
}

void ResetMemoryMetrics( )
{
	s_Metrics.TotalAllocatedBytes.Store( 0, MemoryOrder::Release );
	s_Metrics.TotalFreedBytes.Store( 0, MemoryOrder::Release );
	s_Metrics.CurrentLiveBytes.Store( 0, MemoryOrder::Release );
	s_Metrics.PeakLiveBytes.Store( 0, MemoryOrder::Release );
	s_Metrics.AllocationCount.Store( 0, MemoryOrder::Release );
	s_Metrics.FreeCount.Store( 0, MemoryOrder::Release );
	s_Metrics.ReallocationCount.Store( 0, MemoryOrder::Release );
}

void RecordMemoryAllocation( Size size )
{
	s_Metrics.TotalAllocatedBytes.FetchAdd( size, MemoryOrder::Relaxed );
	const Uint64 current = s_Metrics.CurrentLiveBytes.FetchAdd( size, MemoryOrder::Relaxed ) + size;
	s_Metrics.AllocationCount.FetchAdd( 1, MemoryOrder::Relaxed );
	UpdatePeak( s_Metrics.PeakLiveBytes, current );
}

void RecordMemoryFree( Size size )
{
	s_Metrics.TotalFreedBytes.FetchAdd( size, MemoryOrder::Relaxed );
	s_Metrics.CurrentLiveBytes.FetchSub( size, MemoryOrder::Relaxed );
	s_Metrics.FreeCount.FetchAdd( 1, MemoryOrder::Relaxed );
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
	s_Metrics.ReallocationCount.FetchAdd( 1, MemoryOrder::Relaxed );
}

MemoryMetricsSnapshot GetMemoryMetricsSnapshot( )
{
	MemoryMetricsSnapshot snapshot = { };
	snapshot.TotalAllocatedBytes = s_Metrics.TotalAllocatedBytes.Load( MemoryOrder::Acquire );
	snapshot.TotalFreedBytes = s_Metrics.TotalFreedBytes.Load( MemoryOrder::Acquire );
	snapshot.CurrentLiveBytes = s_Metrics.CurrentLiveBytes.Load( MemoryOrder::Acquire );
	snapshot.PeakLiveBytes = s_Metrics.PeakLiveBytes.Load( MemoryOrder::Acquire );
	snapshot.AllocationCount = s_Metrics.AllocationCount.Load( MemoryOrder::Acquire );
	snapshot.FreeCount = s_Metrics.FreeCount.Load( MemoryOrder::Acquire );
	snapshot.ReallocationCount = s_Metrics.ReallocationCount.Load( MemoryOrder::Acquire );
	return snapshot;
}
} // namespace Blue

#include <Blue/Memory/Metrics/MemoryThreadContext.h>
#include <Blue/Memory/Oom/OomReporter.h>
#include <Blue/System/Threading/Atomic.h>
#include <Blue/System/Time.h>

namespace Blue
{
namespace
{
OomReport s_DefaultReports[ BlueDefaultOomReportCapacity ] = { };
OomReport* s_Reports = s_DefaultReports;
Size s_ReportCapacity = BlueDefaultOomReportCapacity;
AtomicUint64 s_ReportSequence( 0 );

OomReport MakeReport( const AllocationFailureInfo& info, Uint64 sequence ) noexcept
{
  MemoryThreadContext* thread = GetCurrentMemoryThreadContext( );
  OomReport report = { };
  report.SequenceId = sequence;
  report.TimestampTicks = GetTimeNowNs( );
  report.NativeThreadId = thread ? thread->NativeThreadId : 0;
  report.ThreadIndex = thread ? thread->ThreadIndex : 0;
  report.ThreadName = thread ? thread->ThreadName : nullptr;
  report.Pool = info.Pool;
  report.Allocator = info.Allocator;
  report.Tag = info.Tag;
  report.RequestedSize = info.RequestedSize;
  report.RequestedAlignment = info.RequestedAlignment;
  report.PoolBudgetBytes = info.PoolBudgetBytes;
  report.PoolCurrentBytes = info.PoolCurrentBytes;
  report.PoolPeakBytes = info.PoolPeakBytes;
  report.Reason = info.Reason;
  report.Location = info.Location;
  return report;
}
} // namespace

void ConfigureOomReporter( OomReport* externalBuffer, Size capacity ) noexcept
{
  if ( externalBuffer && capacity > 0 )
  {
    s_Reports = externalBuffer;
    s_ReportCapacity = capacity;
  }
  else
  {
    s_Reports = s_DefaultReports;
    s_ReportCapacity = BlueDefaultOomReportCapacity;
  }

  ClearOomReports( );
}

void ClearOomReports( ) noexcept
{
  for ( Size index = 0; index < s_ReportCapacity; ++index )
  {
    s_Reports[ index ] = { };
  }

  s_ReportSequence.Store( 0, MemoryOrder::Release );
}

void RecordOomReport( const AllocationFailureInfo& info ) noexcept
{
  if ( !s_Reports || s_ReportCapacity == 0 )
  {
    return;
  }

  const Uint64 sequence = s_ReportSequence.FetchAdd( 1, MemoryOrder::AcquireRelease );
  const Size index = static_cast< Size >( sequence % s_ReportCapacity );
  s_Reports[ index ] = MakeReport( info, sequence + 1 );
}

Size CaptureOomReports( OomReport* output, Size capacity ) noexcept
{
  if ( !output || capacity == 0 || !s_Reports || s_ReportCapacity == 0 )
  {
    return 0;
  }

  const Uint64 sequence = s_ReportSequence.Load( MemoryOrder::Acquire );
  const Size available = sequence < s_ReportCapacity ? static_cast< Size >( sequence ) : s_ReportCapacity;
  const Size count = available < capacity ? available : capacity;

  for ( Size index = 0; index < count; ++index )
  {
    output[ index ] = s_Reports[ index ];
  }

  return count;
}
} // namespace Blue

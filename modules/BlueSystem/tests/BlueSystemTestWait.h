#pragma once

#include <Blue/System/Threading/Processor.h>
#include <Blue/System/Time.h>
#include <Blue/System/Types.h>

namespace BlueSystemTest
{
inline constexpr Blue::TimeDuration DefaultWaitTimeout = { 5ull * Blue::NanosecondsPerSecond };
inline constexpr Blue::Uint32 DefaultSpinIterationCount = 128;
inline constexpr Blue::Uint32 DefaultYieldIterationCount = 64;
inline constexpr Blue::Uint32 DefaultSleepMilliseconds = 1;

struct WaitOptions final
{
  Blue::TimeDuration Timeout = DefaultWaitTimeout;
  Blue::Uint32 SpinIterationCount = DefaultSpinIterationCount;
  Blue::Uint32 YieldIterationCount = DefaultYieldIterationCount;
  Blue::Uint32 SleepMilliseconds = DefaultSleepMilliseconds;
};

template< typename Predicate >
bool WaitUntil( Predicate predicate, WaitOptions options = { } )
{
  if ( predicate( ) )
  {
    return true;
  }

  if ( Blue::IsTimeDurationZero( options.Timeout ) )
  {
    return predicate( );
  }

  const Blue::TimePoint deadline = Blue::GetTimePointNow( ) + options.Timeout;

  for ( Blue::Uint32 iteration = 0; iteration < options.SpinIterationCount; ++iteration )
  {
    Blue::ProcessorPause( );

    if ( predicate( ) )
    {
      return true;
    }

    if ( ( iteration & 15u ) == 15u && Blue::GetTimePointNow( ) >= deadline )
    {
      return predicate( );
    }
  }

  for ( Blue::Uint32 iteration = 0; iteration < options.YieldIterationCount; ++iteration )
  {
    Blue::YieldThread( );

    if ( predicate( ) )
    {
      return true;
    }

    if ( ( iteration & 7u ) == 7u && Blue::GetTimePointNow( ) >= deadline )
    {
      return predicate( );
    }
  }

  while ( Blue::GetTimePointNow( ) < deadline )
  {
    Blue::SleepCurrentThread( options.SleepMilliseconds );

    if ( predicate( ) )
    {
      return true;
    }
  }

  return predicate( );
}

template< typename Predicate >
bool WaitUntilForMilliseconds( Predicate predicate, Blue::Uint64 timeoutMilliseconds )
{
  WaitOptions options;
  options.Timeout = Blue::MakeTimeDurationFromMilliseconds( timeoutMilliseconds );
  return WaitUntil( predicate, options );
}
} // namespace BlueSystemTest

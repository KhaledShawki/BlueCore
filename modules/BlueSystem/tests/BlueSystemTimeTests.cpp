#include <Blue/System/Time.h>
#include <Blue/System/Types.h>

#include <gtest/gtest.h>

TEST( BlueSystemTimeTests, PlatformClockReportsValidRuntimeValues )
{
  EXPECT_NE( Blue::GetPerformanceFrequency( ), 0u );
  EXPECT_NE( Blue::GetTimeNowNs( ), 0u );
}

TEST( BlueSystemTimeTests, DurationFactoriesConvertToNanoseconds )
{
  EXPECT_EQ( Blue::MakeTimeDurationFromSeconds( 1 ).Nanoseconds, Blue::NanosecondsPerSecond );
  EXPECT_EQ( Blue::MakeTimeDurationFromMilliseconds( 1 ).Nanoseconds, Blue::NanosecondsPerMillisecond );
  EXPECT_EQ( Blue::MakeTimeDurationFromMicroseconds( 1 ).Nanoseconds, Blue::NanosecondsPerMicrosecond );
}

TEST( BlueSystemTimeTests, ElapsedTimeSaturatesWhenEndPrecedesBegin )
{
  const Blue::TimePoint begin{ 100 };
  const Blue::TimePoint end{ 250 };

  EXPECT_EQ( Blue::GetElapsedTime( begin, end ).Nanoseconds, 150u );
  EXPECT_EQ( Blue::GetElapsedTime( end, begin ).Nanoseconds, 0u );
}

TEST( BlueSystemTimeTests, DurationArithmeticSaturatesOnUnderflowAndOverflow )
{
  const Blue::TimeDuration added =
    Blue::MakeTimeDurationFromNanoseconds( 10 ) + Blue::MakeTimeDurationFromNanoseconds( 20 );
  EXPECT_EQ( added.Nanoseconds, 30u );

  const Blue::TimeDuration subtracted =
    Blue::MakeTimeDurationFromNanoseconds( 10 ) - Blue::MakeTimeDurationFromNanoseconds( 20 );
  EXPECT_EQ( subtracted.Nanoseconds, 0u );

  constexpr Blue::Uint64 MaxUint64 = ~static_cast< Blue::Uint64 >( 0 );

  EXPECT_EQ( Blue::MakeTimeDurationFromSeconds( MaxUint64 ).Nanoseconds, MaxUint64 );

  const Blue::TimeDuration saturatedAddition =
    Blue::MakeTimeDurationFromNanoseconds( MaxUint64 ) + Blue::MakeTimeDurationFromNanoseconds( 1 );
  EXPECT_EQ( saturatedAddition.Nanoseconds, MaxUint64 );
}

TEST( BlueSystemTimeTests, TimePointArithmeticSaturatesOnOverflow )
{
  constexpr Blue::Uint64 MaxUint64 = ~static_cast< Blue::Uint64 >( 0 );

  const Blue::TimePoint saturatedPoint = Blue::TimePoint{ MaxUint64 } + Blue::MakeTimeDurationFromNanoseconds( 1 );

  EXPECT_EQ( saturatedPoint.Nanoseconds, MaxUint64 );
}

TEST( BlueSystemTimeTests, StopwatchAdvancesAndRestartKeepsValidStartPoint )
{
  Blue::Stopwatch stopwatch = Blue::StartStopwatch( );
  Blue::TimeDuration elapsed{ };

  for ( int attempt = 0; attempt < 100000; ++attempt )
  {
    elapsed = Blue::GetElapsedTime( stopwatch );

    if ( elapsed.Nanoseconds != 0 )
    {
      break;
    }
  }

  EXPECT_NE( elapsed.Nanoseconds, 0u );

  const Blue::TimeDuration restartElapsed = Blue::RestartStopwatch( stopwatch );
  ( void ) restartElapsed;

  EXPECT_TRUE( Blue::IsTimePointValid( stopwatch.Start ) );
}

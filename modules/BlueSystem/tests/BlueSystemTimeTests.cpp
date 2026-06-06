#include <Blue/System/Time.h>

#include <stdio.h>

namespace
{
int Fail( const char* message )
{
	printf( "BlueSystem time test failed: %s\n", message );
	return 1;
}
} // namespace

int main( )
{
	const Blue::Uint64 frequency = Blue::GetPerformanceFrequency( );

	if ( frequency == 0 )
	{
		return Fail( "performance frequency must be non-zero" );
	}

	const Blue::Uint64 nowNs = Blue::GetTimeNowNs( );

	if ( nowNs == 0 )
	{
		return Fail( "current monotonic time must be non-zero" );
	}

	const Blue::TimeDuration oneSecond = Blue::MakeTimeDurationFromSeconds( 1 );
	const Blue::TimeDuration oneMillisecond = Blue::MakeTimeDurationFromMilliseconds( 1 );
	const Blue::TimeDuration oneMicrosecond = Blue::MakeTimeDurationFromMicroseconds( 1 );

	if ( oneSecond.Nanoseconds != Blue::NanosecondsPerSecond )
	{
		return Fail( "second conversion is incorrect" );
	}

	if ( oneMillisecond.Nanoseconds != Blue::NanosecondsPerMillisecond )
	{
		return Fail( "millisecond conversion is incorrect" );
	}

	if ( oneMicrosecond.Nanoseconds != Blue::NanosecondsPerMicrosecond )
	{
		return Fail( "microsecond conversion is incorrect" );
	}

	const Blue::TimePoint begin{ 100 };
	const Blue::TimePoint end{ 250 };
	const Blue::TimeDuration elapsed = Blue::GetElapsedTime( begin, end );

	if ( elapsed.Nanoseconds != 150 )
	{
		return Fail( "elapsed time subtraction is incorrect" );
	}

	const Blue::TimeDuration saturated = Blue::GetElapsedTime( end, begin );

	if ( saturated.Nanoseconds != 0 )
	{
		return Fail( "negative elapsed time must saturate to zero" );
	}

	const Blue::TimeDuration added =
	    Blue::MakeTimeDurationFromNanoseconds( 10 ) + Blue::MakeTimeDurationFromNanoseconds( 20 );

	if ( added.Nanoseconds != 30 )
	{
		return Fail( "duration addition is incorrect" );
	}

	const Blue::TimeDuration subtracted =
	    Blue::MakeTimeDurationFromNanoseconds( 10 ) - Blue::MakeTimeDurationFromNanoseconds( 20 );

	if ( subtracted.Nanoseconds != 0 )
	{
		return Fail( "duration subtraction must saturate to zero" );
	}

	constexpr Blue::Uint64 maxUint64 = ~static_cast< Blue::Uint64 >( 0 );

	if ( Blue::MakeTimeDurationFromSeconds( maxUint64 ).Nanoseconds != maxUint64 )
	{
		return Fail( "duration factory overflow must saturate" );
	}

	const Blue::TimeDuration saturatedAddition =
	    Blue::MakeTimeDurationFromNanoseconds( maxUint64 ) + Blue::MakeTimeDurationFromNanoseconds( 1 );

	if ( saturatedAddition.Nanoseconds != maxUint64 )
	{
		return Fail( "duration addition overflow must saturate" );
	}

	const Blue::TimePoint saturatedPoint = Blue::TimePoint{ maxUint64 } + Blue::MakeTimeDurationFromNanoseconds( 1 );

	if ( saturatedPoint.Nanoseconds != maxUint64 )
	{
		return Fail( "time point addition overflow must saturate" );
	}

	Blue::Stopwatch stopwatch = Blue::StartStopwatch( );
	Blue::TimeDuration stopwatchElapsed{ };

	for ( int attempt = 0; attempt < 100000; ++attempt )
	{
		stopwatchElapsed = Blue::GetElapsedTime( stopwatch );

		if ( stopwatchElapsed.Nanoseconds != 0 )
		{
			break;
		}
	}

	if ( stopwatchElapsed.Nanoseconds == 0 )
	{
		return Fail( "stopwatch did not advance" );
	}

	const Blue::TimeDuration restartElapsed = Blue::RestartStopwatch( stopwatch );

	if ( Blue::IsTimePointValid( stopwatch.Start ) == false )
	{
		return Fail( "restart must preserve a valid stopwatch start point" );
	}

	( void ) restartElapsed;

	printf( "BlueSystem time tests passed.\n" );
	return 0;
}

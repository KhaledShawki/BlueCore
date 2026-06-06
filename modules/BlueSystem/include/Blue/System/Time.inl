#pragma once

namespace Blue
{
namespace Internal
{
BLUE_FORCE_INLINE constexpr Uint64 GetMaxUint64( ) noexcept
{
	return ~static_cast< Uint64 >( 0 );
}

BLUE_FORCE_INLINE constexpr Uint64 SaturatingAddUint64( Uint64 left, Uint64 right ) noexcept
{
	const Uint64 maxValue = GetMaxUint64( );

	if ( maxValue - left < right )
	{
		return maxValue;
	}

	return left + right;
}

BLUE_FORCE_INLINE constexpr Uint64 SaturatingMultiplyUint64( Uint64 left, Uint64 right ) noexcept
{
	if ( left == 0 || right == 0 )
	{
		return 0;
	}

	const Uint64 maxValue = GetMaxUint64( );
	if ( left > maxValue / right )
	{
		return maxValue;
	}

	return left * right;
}
} // namespace Internal

BLUE_FORCE_INLINE TimeDuration GetElapsedTimeSince( TimePoint begin ) noexcept
{
	return GetElapsedTime( begin, GetTimePointNow( ) );
}

BLUE_FORCE_INLINE TimeDuration MakeTimeDurationFromNanoseconds( Uint64 nanoseconds ) noexcept
{
	return TimeDuration{ nanoseconds };
}

BLUE_FORCE_INLINE TimeDuration MakeTimeDurationFromMicroseconds( Uint64 microseconds ) noexcept
{
	return TimeDuration{ Internal::SaturatingMultiplyUint64( microseconds, NanosecondsPerMicrosecond ) };
}

BLUE_FORCE_INLINE TimeDuration MakeTimeDurationFromMilliseconds( Uint64 milliseconds ) noexcept
{
	return TimeDuration{ Internal::SaturatingMultiplyUint64( milliseconds, NanosecondsPerMillisecond ) };
}

BLUE_FORCE_INLINE TimeDuration MakeTimeDurationFromSeconds( Uint64 seconds ) noexcept
{
	return TimeDuration{ Internal::SaturatingMultiplyUint64( seconds, NanosecondsPerSecond ) };
}

BLUE_FORCE_INLINE Float64 TimeDurationToSeconds( TimeDuration duration ) noexcept
{
	return static_cast< Float64 >( duration.Nanoseconds ) / static_cast< Float64 >( NanosecondsPerSecond );
}

BLUE_FORCE_INLINE Float64 TimeDurationToMilliseconds( TimeDuration duration ) noexcept
{
	return static_cast< Float64 >( duration.Nanoseconds ) / static_cast< Float64 >( NanosecondsPerMillisecond );
}

BLUE_FORCE_INLINE Float64 TimeDurationToMicroseconds( TimeDuration duration ) noexcept
{
	return static_cast< Float64 >( duration.Nanoseconds ) / static_cast< Float64 >( NanosecondsPerMicrosecond );
}

BLUE_FORCE_INLINE Bool IsTimePointValid( TimePoint timePoint ) noexcept
{
	return timePoint.Nanoseconds != 0;
}

BLUE_FORCE_INLINE Bool IsTimeDurationZero( TimeDuration duration ) noexcept
{
	return duration.Nanoseconds == 0;
}

BLUE_FORCE_INLINE Stopwatch StartStopwatch( ) noexcept
{
	return Stopwatch{ GetTimePointNow( ) };
}

BLUE_FORCE_INLINE TimeDuration GetElapsedTime( const Stopwatch& stopwatch ) noexcept
{
	return GetElapsedTimeSince( stopwatch.Start );
}

BLUE_FORCE_INLINE TimeDuration RestartStopwatch( Stopwatch& stopwatch ) noexcept
{
	const TimePoint now = GetTimePointNow( );
	const TimeDuration elapsed = GetElapsedTime( stopwatch.Start, now );
	stopwatch.Start = now;
	return elapsed;
}

BLUE_FORCE_INLINE Bool operator==( TimePoint left, TimePoint right ) noexcept
{
	return left.Nanoseconds == right.Nanoseconds;
}

BLUE_FORCE_INLINE Bool operator!=( TimePoint left, TimePoint right ) noexcept
{
	return !( left == right );
}

BLUE_FORCE_INLINE Bool operator<( TimePoint left, TimePoint right ) noexcept
{
	return left.Nanoseconds < right.Nanoseconds;
}

BLUE_FORCE_INLINE Bool operator<=( TimePoint left, TimePoint right ) noexcept
{
	return left.Nanoseconds <= right.Nanoseconds;
}

BLUE_FORCE_INLINE Bool operator>( TimePoint left, TimePoint right ) noexcept
{
	return left.Nanoseconds > right.Nanoseconds;
}

BLUE_FORCE_INLINE Bool operator>=( TimePoint left, TimePoint right ) noexcept
{
	return left.Nanoseconds >= right.Nanoseconds;
}

BLUE_FORCE_INLINE Bool operator==( TimeDuration left, TimeDuration right ) noexcept
{
	return left.Nanoseconds == right.Nanoseconds;
}

BLUE_FORCE_INLINE Bool operator!=( TimeDuration left, TimeDuration right ) noexcept
{
	return !( left == right );
}

BLUE_FORCE_INLINE Bool operator<( TimeDuration left, TimeDuration right ) noexcept
{
	return left.Nanoseconds < right.Nanoseconds;
}

BLUE_FORCE_INLINE Bool operator<=( TimeDuration left, TimeDuration right ) noexcept
{
	return left.Nanoseconds <= right.Nanoseconds;
}

BLUE_FORCE_INLINE Bool operator>( TimeDuration left, TimeDuration right ) noexcept
{
	return left.Nanoseconds > right.Nanoseconds;
}

BLUE_FORCE_INLINE Bool operator>=( TimeDuration left, TimeDuration right ) noexcept
{
	return left.Nanoseconds >= right.Nanoseconds;
}

BLUE_FORCE_INLINE TimePoint operator+( TimePoint timePoint, TimeDuration duration ) noexcept
{
	return TimePoint{ Internal::SaturatingAddUint64( timePoint.Nanoseconds, duration.Nanoseconds ) };
}

BLUE_FORCE_INLINE TimeDuration operator-( TimePoint end, TimePoint begin ) noexcept
{
	return GetElapsedTime( begin, end );
}

BLUE_FORCE_INLINE TimeDuration operator+( TimeDuration left, TimeDuration right ) noexcept
{
	return TimeDuration{ Internal::SaturatingAddUint64( left.Nanoseconds, right.Nanoseconds ) };
}

BLUE_FORCE_INLINE TimeDuration operator-( TimeDuration left, TimeDuration right ) noexcept
{
	if ( left.Nanoseconds <= right.Nanoseconds )
	{
		return TimeDuration{ 0 };
	}

	return TimeDuration{ left.Nanoseconds - right.Nanoseconds };
}
} // namespace Blue

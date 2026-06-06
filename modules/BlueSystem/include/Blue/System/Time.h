#pragma once

#include <Blue/System/Api.h>
#include <Blue/System/Compiler.h>
#include <Blue/System/Types.h>

namespace Blue
{
constexpr Uint64 NanosecondsPerMicrosecond = 1000ull;
constexpr Uint64 NanosecondsPerMillisecond = 1000ull * NanosecondsPerMicrosecond;
constexpr Uint64 NanosecondsPerSecond = 1000ull * NanosecondsPerMillisecond;
constexpr Uint64 MicrosecondsPerMillisecond = 1000ull;
constexpr Uint64 MillisecondsPerSecond = 1000ull;

struct TimePoint final
{
	Uint64 Nanoseconds = 0;
};

struct TimeDuration final
{
	Uint64 Nanoseconds = 0;
};

struct Stopwatch final
{
	TimePoint Start;
};

BLUE_SYSTEM_API Uint64 GetPerformanceCounter( ) noexcept;
BLUE_SYSTEM_API Uint64 GetPerformanceFrequency( ) noexcept;
BLUE_SYSTEM_API Uint64 ConvertPerformanceCounterToNanoseconds( Uint64 counter ) noexcept;

BLUE_SYSTEM_API Uint64 GetTimeNowNs( ) noexcept;
BLUE_SYSTEM_API TimePoint GetTimePointNow( ) noexcept;

BLUE_SYSTEM_API TimeDuration GetElapsedTime( TimePoint begin, TimePoint end ) noexcept;
TimeDuration GetElapsedTimeSince( TimePoint begin ) noexcept;

TimeDuration MakeTimeDurationFromNanoseconds( Uint64 nanoseconds ) noexcept;
TimeDuration MakeTimeDurationFromMicroseconds( Uint64 microseconds ) noexcept;
TimeDuration MakeTimeDurationFromMilliseconds( Uint64 milliseconds ) noexcept;
TimeDuration MakeTimeDurationFromSeconds( Uint64 seconds ) noexcept;

Float64 TimeDurationToSeconds( TimeDuration duration ) noexcept;
Float64 TimeDurationToMilliseconds( TimeDuration duration ) noexcept;
Float64 TimeDurationToMicroseconds( TimeDuration duration ) noexcept;

Bool IsTimePointValid( TimePoint timePoint ) noexcept;
Bool IsTimeDurationZero( TimeDuration duration ) noexcept;

Stopwatch StartStopwatch( ) noexcept;
TimeDuration GetElapsedTime( const Stopwatch& stopwatch ) noexcept;
TimeDuration RestartStopwatch( Stopwatch& stopwatch ) noexcept;

Bool operator==( TimePoint left, TimePoint right ) noexcept;
Bool operator!=( TimePoint left, TimePoint right ) noexcept;
Bool operator<( TimePoint left, TimePoint right ) noexcept;
Bool operator<=( TimePoint left, TimePoint right ) noexcept;
Bool operator>( TimePoint left, TimePoint right ) noexcept;
Bool operator>=( TimePoint left, TimePoint right ) noexcept;

Bool operator==( TimeDuration left, TimeDuration right ) noexcept;
Bool operator!=( TimeDuration left, TimeDuration right ) noexcept;
Bool operator<( TimeDuration left, TimeDuration right ) noexcept;
Bool operator<=( TimeDuration left, TimeDuration right ) noexcept;
Bool operator>( TimeDuration left, TimeDuration right ) noexcept;
Bool operator>=( TimeDuration left, TimeDuration right ) noexcept;

TimePoint operator+( TimePoint timePoint, TimeDuration duration ) noexcept;
TimeDuration operator-( TimePoint end, TimePoint begin ) noexcept;
TimeDuration operator+( TimeDuration left, TimeDuration right ) noexcept;
TimeDuration operator-( TimeDuration left, TimeDuration right ) noexcept;
} // namespace Blue

#include <Blue/System/Time.inl>

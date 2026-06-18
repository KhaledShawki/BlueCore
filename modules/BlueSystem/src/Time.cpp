#include <Blue/System/Platform.h>
#include <Blue/System/Time.h>

#if BLUE_PLATFORM == BLUE_PLATFORM_WINDOWS
#  include <Blue/System/Platform/WindowsLean.h>
#elif BLUE_PLATFORM == BLUE_PLATFORM_LINUX || BLUE_PLATFORM == BLUE_PLATFORM_MACOS
#  include <time.h>
#endif

namespace Blue
{
namespace
{
constexpr Uint64 MaxUint64 = ~static_cast< Uint64 >( 0 );

#if BLUE_PLATFORM == BLUE_PLATFORM_WINDOWS
Uint64 QueryPerformanceFrequencyCached( ) noexcept
{
  static const Uint64 frequency = []( ) noexcept -> Uint64
  {
    LARGE_INTEGER value;

    if ( !QueryPerformanceFrequency( &value ) )
    {
      return 0;
    }

    return static_cast< Uint64 >( value.QuadPart );
  }( );

  return frequency;
}
#endif

#if BLUE_PLATFORM == BLUE_PLATFORM_LINUX || BLUE_PLATFORM == BLUE_PLATFORM_MACOS
Uint64 SaturatingMultiply( Uint64 left, Uint64 right ) noexcept
{
  if ( left == 0 || right == 0 )
  {
    return 0;
  }

  if ( left > MaxUint64 / right )
  {
    return MaxUint64;
  }

  return left * right;
}

Uint64 SaturatingAdd( Uint64 left, Uint64 right ) noexcept
{
  if ( MaxUint64 - left < right )
  {
    return MaxUint64;
  }

  return left + right;
}
#endif
} // namespace

Uint64 GetPerformanceCounter( ) noexcept
{
#if BLUE_PLATFORM == BLUE_PLATFORM_WINDOWS
  LARGE_INTEGER value;

  if ( !QueryPerformanceCounter( &value ) )
  {
    return 0;
  }

  return static_cast< Uint64 >( value.QuadPart );
#elif BLUE_PLATFORM == BLUE_PLATFORM_LINUX || BLUE_PLATFORM == BLUE_PLATFORM_MACOS
  timespec value{ };

  if ( clock_gettime( CLOCK_MONOTONIC, &value ) != 0 )
  {
    return 0;
  }

  return SaturatingAdd( SaturatingMultiply( static_cast< Uint64 >( value.tv_sec ), NanosecondsPerSecond ),
                        static_cast< Uint64 >( value.tv_nsec ) );
#else
  return 0;
#endif
}

Uint64 GetPerformanceFrequency( ) noexcept
{
#if BLUE_PLATFORM == BLUE_PLATFORM_WINDOWS
  return QueryPerformanceFrequencyCached( );
#elif BLUE_PLATFORM == BLUE_PLATFORM_LINUX || BLUE_PLATFORM == BLUE_PLATFORM_MACOS
  return NanosecondsPerSecond;
#else
  return 0;
#endif
}

Uint64 ConvertPerformanceCounterToNanoseconds( Uint64 counter ) noexcept
{
#if BLUE_PLATFORM == BLUE_PLATFORM_WINDOWS
  const Uint64 frequency = GetPerformanceFrequency( );

  if ( frequency == 0 )
  {
    return 0;
  }

  const long double seconds = static_cast< long double >( counter ) / static_cast< long double >( frequency );
  const long double nanoseconds = seconds * static_cast< long double >( NanosecondsPerSecond );

  if ( nanoseconds >= static_cast< long double >( MaxUint64 ) )
  {
    return MaxUint64;
  }

  return static_cast< Uint64 >( nanoseconds );
#elif BLUE_PLATFORM == BLUE_PLATFORM_LINUX || BLUE_PLATFORM == BLUE_PLATFORM_MACOS
  return counter;
#else
  return 0;
#endif
}

Uint64 GetTimeNowNs( ) noexcept
{
  return ConvertPerformanceCounterToNanoseconds( GetPerformanceCounter( ) );
}

TimePoint GetTimePointNow( ) noexcept
{
  return TimePoint{ GetTimeNowNs( ) };
}

TimeDuration GetElapsedTime( TimePoint begin, TimePoint end ) noexcept
{
  if ( end.Nanoseconds <= begin.Nanoseconds )
  {
    return TimeDuration{ 0 };
  }

  return TimeDuration{ end.Nanoseconds - begin.Nanoseconds };
}
} // namespace Blue

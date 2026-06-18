#include <Blue/System/Threading/Processor.h>
#include <Blue/System/Types.h>

#include <errno.h>
#include <sched.h>
#include <time.h>
#include <unistd.h>

namespace Blue
{
Uint32 GetLogicalProcessorCount( ) noexcept
{
  const long value = sysconf( _SC_NPROCESSORS_ONLN );

  if ( value <= 0 )
  {
    return 1;
  }

  return static_cast< Uint32 >( value );
}

Uint32 GetCacheLineSize( ) noexcept
{
#if defined( _SC_LEVEL1_DCACHE_LINESIZE )
  const long value = sysconf( _SC_LEVEL1_DCACHE_LINESIZE );

  if ( value > 0 )
  {
    return static_cast< Uint32 >( value );
  }
#endif

  return DefaultCacheLineSize;
}

void YieldThread( )
{
  sched_yield( );
}

void SleepCurrentThread( Uint32 milliseconds )
{
  timespec request;
  request.tv_sec = static_cast< time_t >( milliseconds / 1000 );
  request.tv_nsec = static_cast< long >( ( milliseconds % 1000 ) * 1000000u );

  while ( nanosleep( &request, &request ) != 0 && errno == EINTR )
  {
  }
}
} // namespace Blue

#include <Blue/System/Threading/Processor.h>
#include <Blue/System/Types.h>

#include <errno.h>
#include <sched.h>
#include <stddef.h>
#include <sys/sysctl.h>
#include <time.h>

namespace Blue
{
namespace
{
Uint32 QuerySysctlUint32( const char* name, Uint32 fallback ) noexcept
{
  int value = 0;
  size_t size = sizeof( value );

  if ( sysctlbyname( name, &value, &size, nullptr, 0 ) != 0 || value <= 0 )
  {
    return fallback;
  }

  return static_cast< Uint32 >( value );
}
} // namespace

Uint32 GetLogicalProcessorCount( ) noexcept
{
  return QuerySysctlUint32( "hw.logicalcpu", 1 );
}

Uint32 GetCacheLineSize( ) noexcept
{
  return QuerySysctlUint32( "hw.cachelinesize", DefaultCacheLineSize );
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

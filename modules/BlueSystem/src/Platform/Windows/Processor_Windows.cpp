#include <Blue/System/Platform/WindowsLean.h>
#include <Blue/System/Threading/Processor.h>
#include <Blue/System/Types.h>

namespace Blue
{
namespace
{
Uint32 QueryWindowsCacheLineSize( ) noexcept
{
  SYSTEM_LOGICAL_PROCESSOR_INFORMATION entries[ 64 ] = { };
  DWORD bufferSize = static_cast< DWORD >( sizeof( entries ) );

  if ( !GetLogicalProcessorInformation( entries, &bufferSize ) )
  {
    return DefaultCacheLineSize;
  }

  const DWORD entryCount = bufferSize / static_cast< DWORD >( sizeof( SYSTEM_LOGICAL_PROCESSOR_INFORMATION ) );

  for ( DWORD index = 0; index < entryCount; ++index )
  {
    const SYSTEM_LOGICAL_PROCESSOR_INFORMATION& entry = entries[ index ];

    if ( entry.Relationship == RelationCache && entry.Cache.Level == 1 && entry.Cache.LineSize > 0 )
    {
      return static_cast< Uint32 >( entry.Cache.LineSize );
    }
  }

  return DefaultCacheLineSize;
}
} // namespace

Uint32 GetLogicalProcessorCount( ) noexcept
{
  SYSTEM_INFO info = { };
  GetNativeSystemInfo( &info );

  if ( info.dwNumberOfProcessors == 0 )
  {
    return 1;
  }

  return static_cast< Uint32 >( info.dwNumberOfProcessors );
}

Uint32 GetCacheLineSize( ) noexcept
{
  return QueryWindowsCacheLineSize( );
}

void YieldThread( )
{
  SwitchToThread( );
}

void SleepCurrentThread( Uint32 milliseconds )
{
  Sleep( milliseconds );
}
} // namespace Blue

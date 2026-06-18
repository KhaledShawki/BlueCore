#include <Blue/System/Threading/Processor.h>

#include <stdio.h>

namespace
{
int Fail( const char* message )
{
  printf( "FAILED: %s\n", message );
  return 1;
}

int TestProcessorInfo( )
{
  const Blue::ProcessorArchitecture architecture = Blue::GetProcessorArchitecture( );
  const char* architectureName = Blue::GetProcessorArchitectureName( architecture );

  if ( !architectureName || architectureName[ 0 ] == '\0' )
  {
    return Fail( "processor architecture name must be available" );
  }

  const Blue::Uint32 logicalProcessorCount = Blue::GetLogicalProcessorCount( );
  if ( logicalProcessorCount == 0 )
  {
    return Fail( "logical processor count must be greater than zero" );
  }

  const Blue::Uint32 cacheLineSize = Blue::GetCacheLineSize( );
  if ( cacheLineSize == 0 )
  {
    return Fail( "cache line size must be greater than zero" );
  }

  if ( ( cacheLineSize & ( cacheLineSize - 1 ) ) != 0 )
  {
    return Fail( "cache line size must be a power of two" );
  }

  const Blue::ProcessorInfo info = Blue::QueryProcessorInfo( );
  if ( info.LogicalProcessorCount == 0 )
  {
    return Fail( "processor info logical processor count must be greater than zero" );
  }

  if ( info.CacheLineSize == 0 )
  {
    return Fail( "processor info cache line size must be greater than zero" );
  }

  if ( !info.ArchitectureName || info.ArchitectureName[ 0 ] == '\0' )
  {
    return Fail( "processor info architecture name must be available" );
  }

  return 0;
}

int TestRecommendedWorkerThreadCount( )
{
  const Blue::Uint32 logicalProcessorCount = Blue::GetLogicalProcessorCount( );
  const Blue::Uint32 recommendedDefault = Blue::GetRecommendedWorkerThreadCount( );
  const Blue::Uint32 recommendedNoneReserved = Blue::GetRecommendedWorkerThreadCount( 0 );
  const Blue::Uint32 recommendedTooManyReserved = Blue::GetRecommendedWorkerThreadCount( logicalProcessorCount + 32 );

  if ( recommendedDefault == 0 )
  {
    return Fail( "default recommended worker count must be greater than zero" );
  }

  if ( recommendedNoneReserved == 0 )
  {
    return Fail( "recommended worker count with no reserved threads must be greater than zero" );
  }

  if ( recommendedTooManyReserved != 1 )
  {
    return Fail( "recommended worker count must clamp to one when too many threads are reserved" );
  }

  return 0;
}
} // namespace

int main( )
{
  if ( TestProcessorInfo( ) != 0 )
  {
    return 1;
  }

  if ( TestRecommendedWorkerThreadCount( ) != 0 )
  {
    return 1;
  }

  Blue::ProcessorPause( );
  Blue::YieldThread( );
  Blue::SleepCurrentThread( 1 );

  printf( "BlueSystem processor tests passed.\n" );
  return 0;
}

#include <Blue/System.h>

#include <stdio.h>

namespace
{
struct TestState
{
  Blue::AtomicUint32 Counter;
};

Blue::Uint32 ThreadEntry( void* userData )
{
  TestState* state = static_cast< TestState* >( userData );

  for ( Blue::Uint32 index = 0; index < 10000; ++index )
  {
    state->Counter.FetchAdd( 1, Blue::MemoryOrder::AcquireRelease );
  }

  return 7;
}

Blue::Uint32 DetachedThreadEntry( void* )
{
  return 0;
}

int Fail( const char* message )
{
  fprintf( stderr, "FAILED: %s\n", message );
  return 1;
}
} // namespace

int main( )
{
  static_assert( sizeof( Blue::Bool ) == 1 );
  static_assert( sizeof( Blue::Char ) == 1 );
  static_assert( sizeof( Blue::UniChar ) == 4 );

  if ( !Blue::IsPowerOfTwo( 64 ) )
  {
    return Fail( "IsPowerOfTwo failed" );
  }

  if ( Blue::AlignUp( 65, 64 ) != 128 )
  {
    return Fail( "AlignUp failed" );
  }

  Blue::Result ok = Blue::Success( );

  if ( !ok.Succeeded( ) || ok.Failed( ) )
  {
    return Fail( "Result failed" );
  }

  Blue::AtomicUint32 value( 0 );
  value.Store( 10, Blue::MemoryOrder::Release );

  if ( value.Load( Blue::MemoryOrder::Acquire ) != 10 )
  {
    return Fail( "AtomicLoad/Store failed" );
  }

  Blue::Uint32 expected = 10;

  if ( !value.CompareExchange( expected, 20, Blue::MemoryOrder::AcquireRelease ) )
  {
    return Fail( "AtomicCompareExchange failed" );
  }

  Blue::SpinLock lock;

  if ( !lock.TryAcquire( ) )
  {
    return Fail( "SpinLock TryLock failed" );
  }

  lock.Release( );

  TestState state = { };
  state.Counter.Store( 0, Blue::MemoryOrder::Relaxed );

  Blue::Thread thread = { };
  Blue::ThreadCreateDesc threadDesc = { };
  threadDesc.Name = "BlueSystemTest";
  threadDesc.Entry = ThreadEntry;
  threadDesc.UserData = &state;

  if ( !Blue::CreateThread( thread, threadDesc ) )
  {
    return Fail( "CreateThread failed" );
  }

  Blue::Uint32 exitCode = 0;

  if ( !Blue::JoinThread( thread, &exitCode ) )
  {
    return Fail( "JoinThread failed" );
  }

  if ( exitCode != 7 )
  {
    return Fail( "Thread exit code failed" );
  }

  if ( state.Counter.Load( Blue::MemoryOrder::Acquire ) != 10000 )
  {
    return Fail( "Thread atomic counter failed" );
  }

  Blue::Thread detached = { };
  Blue::ThreadCreateDesc detachDesc = { };
  detachDesc.Name = "BlueDetachedTest";
  detachDesc.Entry = DetachedThreadEntry;

  if ( !Blue::CreateThread( detached, detachDesc ) )
  {
    return Fail( "CreateThread detach case failed" );
  }

  if ( !Blue::DetachThread( detached ) )
  {
    return Fail( "DetachThread failed" );
  }

  Blue::SleepCurrentThread( 10 );

  printf( "BlueSystem Phase 2 tests passed.\n" );
  return 0;
}

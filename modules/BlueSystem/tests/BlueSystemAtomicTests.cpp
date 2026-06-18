#include <Blue/System/Atomic.h>
#include <Blue/System/SpinLock.h>
#include <Blue/System/Types.h>

#include <stdio.h>
#include <stdlib.h>

#if defined( __linux__ ) || defined( __APPLE__ )
#  include <pthread.h>
#endif

#define BLUE_TEST_EXPECT( expression )                                                                                 \
  do                                                                                                                   \
  {                                                                                                                    \
    if ( !( expression ) )                                                                                             \
    {                                                                                                                  \
      fprintf( stderr, "Test failed: %s at %s:%d\n", #expression, __FILE__, __LINE__ );                                \
      abort( );                                                                                                        \
    }                                                                                                                  \
  }                                                                                                                    \
  while ( false )

namespace
{
enum class TestEnum32 : Blue::Uint32
{
  Zero = 0,
  One = 1,
  Two = 2,
};

struct SpinLockThreadContext
{
  Blue::SpinLock* Lock;
  Blue::Uint32* Counter;
  Blue::Uint32 Iterations;
};

#if defined( __linux__ ) || defined( __APPLE__ )
void* SpinLockThreadEntry( void* userData )
{
  SpinLockThreadContext* context = static_cast< SpinLockThreadContext* >( userData );

  for ( Blue::Uint32 index = 0; index < context->Iterations; ++index )
  {
    Blue::ScopedSpinLock lock( *context->Lock );
    ++( *context->Counter );
  }

  return nullptr;
}
#endif
} // namespace

static void TestAtomicUint32( )
{
  Blue::Atomic< Blue::Uint32 > counter( 7 );

  BLUE_TEST_EXPECT( counter.Load( ) == 7 );
  counter.Store( 10, Blue::MemoryOrder::Release );
  BLUE_TEST_EXPECT( counter.Load( Blue::MemoryOrder::Acquire ) == 10 );
  BLUE_TEST_EXPECT( counter.Exchange( 20 ) == 10 );
  BLUE_TEST_EXPECT( counter.Load( ) == 20 );

  Blue::Uint32 expected = 20;
  BLUE_TEST_EXPECT( counter.CompareExchange( expected, 30 ) );
  BLUE_TEST_EXPECT( expected == 20 );
  BLUE_TEST_EXPECT( counter.Load( ) == 30 );

  expected = 20;
  BLUE_TEST_EXPECT( !counter.CompareExchange( expected, 40 ) );
  BLUE_TEST_EXPECT( expected == 30 );
  BLUE_TEST_EXPECT( counter.Load( ) == 30 );

  BLUE_TEST_EXPECT( counter.FetchAdd( 5 ) == 30 );
  BLUE_TEST_EXPECT( counter.Load( ) == 35 );
  BLUE_TEST_EXPECT( counter.FetchSub( 3 ) == 35 );
  BLUE_TEST_EXPECT( counter.Load( ) == 32 );

  const Blue::Uint16 smallIncrement = 2;
  BLUE_TEST_EXPECT( counter.FetchAdd( smallIncrement ) == 32 );
  BLUE_TEST_EXPECT( counter.Load( ) == 34 );
}

static void TestAtomicInt64( )
{
  Blue::Atomic< Blue::Int64 > value( -10 );

  BLUE_TEST_EXPECT( value.Load( ) == -10 );
  BLUE_TEST_EXPECT( value.FetchAdd( 15 ) == -10 );
  BLUE_TEST_EXPECT( value.Load( ) == 5 );
  BLUE_TEST_EXPECT( value.FetchSub( 7 ) == 5 );
  BLUE_TEST_EXPECT( value.Load( ) == -2 );
}

static void TestAtomicBool( )
{
  Blue::Atomic< Blue::Bool > value( false );

  BLUE_TEST_EXPECT( value.Load( ) == false );
  value.Store( true );
  BLUE_TEST_EXPECT( value.Load( ) == true );

  Blue::Bool expected = true;
  BLUE_TEST_EXPECT( value.CompareExchange( expected, false ) );
  BLUE_TEST_EXPECT( value.Load( ) == false );
}

static void TestAtomicPointer( )
{
  int a = 1;
  int b = 2;

  Blue::Atomic< int* > pointer( &a );

  BLUE_TEST_EXPECT( pointer.Load( ) == &a );
  BLUE_TEST_EXPECT( pointer.Exchange( &b ) == &a );
  BLUE_TEST_EXPECT( pointer.Load( ) == &b );

  int* expected = &b;
  BLUE_TEST_EXPECT( pointer.CompareExchange( expected, &a ) );
  BLUE_TEST_EXPECT( pointer.Load( ) == &a );
}

static void TestAtomicEnum( )
{
  Blue::Atomic< TestEnum32 > value( TestEnum32::One );

  BLUE_TEST_EXPECT( value.Load( ) == TestEnum32::One );
  value.Store( TestEnum32::Two );
  BLUE_TEST_EXPECT( value.Load( ) == TestEnum32::Two );
}

static void TestAtomic128( )
{
  static_assert( sizeof( Blue::AtomicValue128 ) == 16 );
  static_assert( alignof( Blue::AtomicValue128 ) == 16 );
  static_assert( sizeof( Blue::Atomic128 ) == 16 );
  static_assert( alignof( Blue::Atomic128 ) >= 16 );

  Blue::Atomic128 value( Blue::AtomicValue128{ 1, 2 } );

  BLUE_TEST_EXPECT( value.Load( ) == Blue::AtomicValue128( 1, 2 ) );

  value.Store( Blue::AtomicValue128{ 3, 4 } );
  BLUE_TEST_EXPECT( value.Load( ) == Blue::AtomicValue128( 3, 4 ) );

  BLUE_TEST_EXPECT( value.Exchange( Blue::AtomicValue128{ 5, 6 } ) == Blue::AtomicValue128( 3, 4 ) );
  BLUE_TEST_EXPECT( value.Load( ) == Blue::AtomicValue128( 5, 6 ) );

  Blue::AtomicValue128 expected{ 5, 6 };
  BLUE_TEST_EXPECT( value.CompareExchange( expected, Blue::AtomicValue128{ 7, 8 } ) );
  BLUE_TEST_EXPECT( expected == Blue::AtomicValue128( 5, 6 ) );
  BLUE_TEST_EXPECT( value.Load( ) == Blue::AtomicValue128( 7, 8 ) );

  expected = Blue::AtomicValue128{ 5, 6 };
  BLUE_TEST_EXPECT( !value.CompareExchange( expected, Blue::AtomicValue128{ 9, 10 } ) );
  BLUE_TEST_EXPECT( expected == Blue::AtomicValue128( 7, 8 ) );
  BLUE_TEST_EXPECT( value.Load( ) == Blue::AtomicValue128( 7, 8 ) );
}

static void TestSpinLockSingleThread( )
{
  Blue::SpinLock lock;
  BLUE_TEST_EXPECT( lock.TryAcquire( ) );
  lock.Release( );
}

static void TestSpinLockMultiThread( )
{
#if defined( __linux__ ) || defined( __APPLE__ )
  constexpr Blue::Uint32 ThreadCount = 4;
  constexpr Blue::Uint32 Iterations = 10000;

  Blue::SpinLock lock;
  Blue::Uint32 counter = 0;
  pthread_t threads[ ThreadCount ] = { };
  SpinLockThreadContext contexts[ ThreadCount ] = { };

  for ( Blue::Uint32 index = 0; index < ThreadCount; ++index )
  {
    contexts[ index ].Lock = &lock;
    contexts[ index ].Counter = &counter;
    contexts[ index ].Iterations = Iterations;
    BLUE_TEST_EXPECT( pthread_create( &threads[ index ], nullptr, SpinLockThreadEntry, &contexts[ index ] ) == 0 );
  }

  for ( Blue::Uint32 index = 0; index < ThreadCount; ++index )
  {
    BLUE_TEST_EXPECT( pthread_join( threads[ index ], nullptr ) == 0 );
  }

  BLUE_TEST_EXPECT( counter == ThreadCount * Iterations );
#endif
}

int main( )
{
  TestAtomicUint32( );
  TestAtomicInt64( );
  TestAtomicBool( );
  TestAtomicPointer( );
  TestAtomicEnum( );
  TestAtomic128( );
  TestSpinLockSingleThread( );
  TestSpinLockMultiThread( );

  printf( "BlueSystem atomic layer tests passed.\n" );
  return 0;
}

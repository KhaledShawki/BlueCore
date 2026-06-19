#include <Blue/System/Atomic.h>
#include <Blue/System/SpinLock.h>
#include <Blue/System/Types.h>

#include <gtest/gtest.h>


#if defined( __linux__ ) || defined( __APPLE__ )
#  include <pthread.h>
#endif


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

  ASSERT_TRUE( counter.Load( ) == 7 );
  counter.Store( 10, Blue::MemoryOrder::Release );
  ASSERT_TRUE( counter.Load( Blue::MemoryOrder::Acquire ) == 10 );
  ASSERT_TRUE( counter.Exchange( 20 ) == 10 );
  ASSERT_TRUE( counter.Load( ) == 20 );

  Blue::Uint32 expected = 20;
  ASSERT_TRUE( counter.CompareExchange( expected, 30 ) );
  ASSERT_TRUE( expected == 20 );
  ASSERT_TRUE( counter.Load( ) == 30 );

  expected = 20;
  ASSERT_TRUE( !counter.CompareExchange( expected, 40 ) );
  ASSERT_TRUE( expected == 30 );
  ASSERT_TRUE( counter.Load( ) == 30 );

  ASSERT_TRUE( counter.FetchAdd( 5 ) == 30 );
  ASSERT_TRUE( counter.Load( ) == 35 );
  ASSERT_TRUE( counter.FetchSub( 3 ) == 35 );
  ASSERT_TRUE( counter.Load( ) == 32 );

  const Blue::Uint16 smallIncrement = 2;
  ASSERT_TRUE( counter.FetchAdd( smallIncrement ) == 32 );
  ASSERT_TRUE( counter.Load( ) == 34 );
}

static void TestAtomicInt64( )
{
  Blue::Atomic< Blue::Int64 > value( -10 );

  ASSERT_TRUE( value.Load( ) == -10 );
  ASSERT_TRUE( value.FetchAdd( 15 ) == -10 );
  ASSERT_TRUE( value.Load( ) == 5 );
  ASSERT_TRUE( value.FetchSub( 7 ) == 5 );
  ASSERT_TRUE( value.Load( ) == -2 );
}

static void TestAtomicBool( )
{
  Blue::Atomic< Blue::Bool > value( false );

  ASSERT_TRUE( value.Load( ) == false );
  value.Store( true );
  ASSERT_TRUE( value.Load( ) == true );

  Blue::Bool expected = true;
  ASSERT_TRUE( value.CompareExchange( expected, false ) );
  ASSERT_TRUE( value.Load( ) == false );
}

static void TestAtomicPointer( )
{
  int a = 1;
  int b = 2;

  Blue::Atomic< int* > pointer( &a );

  ASSERT_TRUE( pointer.Load( ) == &a );
  ASSERT_TRUE( pointer.Exchange( &b ) == &a );
  ASSERT_TRUE( pointer.Load( ) == &b );

  int* expected = &b;
  ASSERT_TRUE( pointer.CompareExchange( expected, &a ) );
  ASSERT_TRUE( pointer.Load( ) == &a );
}

static void TestAtomicEnum( )
{
  Blue::Atomic< TestEnum32 > value( TestEnum32::One );

  ASSERT_TRUE( value.Load( ) == TestEnum32::One );
  value.Store( TestEnum32::Two );
  ASSERT_TRUE( value.Load( ) == TestEnum32::Two );
}

static void TestAtomic128( )
{
  static_assert( sizeof( Blue::AtomicValue128 ) == 16 );
  static_assert( alignof( Blue::AtomicValue128 ) == 16 );
  static_assert( sizeof( Blue::Atomic128 ) == 16 );
  static_assert( alignof( Blue::Atomic128 ) >= 16 );

  Blue::Atomic128 value( Blue::AtomicValue128{ 1, 2 } );

  ASSERT_TRUE( value.Load( ) == Blue::AtomicValue128( 1, 2 ) );

  value.Store( Blue::AtomicValue128{ 3, 4 } );
  ASSERT_TRUE( value.Load( ) == Blue::AtomicValue128( 3, 4 ) );

  ASSERT_TRUE( value.Exchange( Blue::AtomicValue128{ 5, 6 } ) == Blue::AtomicValue128( 3, 4 ) );
  ASSERT_TRUE( value.Load( ) == Blue::AtomicValue128( 5, 6 ) );

  Blue::AtomicValue128 expected{ 5, 6 };
  ASSERT_TRUE( value.CompareExchange( expected, Blue::AtomicValue128{ 7, 8 } ) );
  ASSERT_TRUE( expected == Blue::AtomicValue128( 5, 6 ) );
  ASSERT_TRUE( value.Load( ) == Blue::AtomicValue128( 7, 8 ) );

  expected = Blue::AtomicValue128{ 5, 6 };
  ASSERT_TRUE( !value.CompareExchange( expected, Blue::AtomicValue128{ 9, 10 } ) );
  ASSERT_TRUE( expected == Blue::AtomicValue128( 7, 8 ) );
  ASSERT_TRUE( value.Load( ) == Blue::AtomicValue128( 7, 8 ) );
}

static void TestSpinLockSingleThread( )
{
  Blue::SpinLock lock;
  ASSERT_TRUE( lock.TryAcquire( ) );
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
    ASSERT_TRUE( pthread_create( &threads[ index ], nullptr, SpinLockThreadEntry, &contexts[ index ] ) == 0 );
  }

  for ( Blue::Uint32 index = 0; index < ThreadCount; ++index )
  {
    ASSERT_TRUE( pthread_join( threads[ index ], nullptr ) == 0 );
  }

  ASSERT_TRUE( counter == ThreadCount * Iterations );
#endif
}

TEST( BlueSystemAtomicTests, RunsSuccessfully )
{
  TestAtomicUint32( );
  TestAtomicInt64( );
  TestAtomicBool( );
  TestAtomicPointer( );
  TestAtomicEnum( );
  TestAtomic128( );
  TestSpinLockSingleThread( );
  TestSpinLockMultiThread( );
}

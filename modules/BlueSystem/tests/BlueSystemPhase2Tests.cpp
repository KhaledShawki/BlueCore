#include <Blue/System.h>

#include <gtest/gtest.h>

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
} // namespace

static_assert( sizeof( Blue::Bool ) == 1 );
static_assert( sizeof( Blue::Char ) == 1 );
static_assert( sizeof( Blue::UniChar ) == 4 );

TEST( BlueSystemPhase2Tests, BaseHelpersAndResultContractsAreStable )
{
  EXPECT_TRUE( Blue::IsPowerOfTwo( 64 ) );
  EXPECT_EQ( Blue::AlignUp( 65, 64 ), 128u );

  const Blue::Result result = Blue::Success( );
  EXPECT_TRUE( result.Succeeded( ) );
  EXPECT_FALSE( result.Failed( ) );
}

TEST( BlueSystemPhase2Tests, AtomicLoadStoreAndCompareExchangeAreStable )
{
  Blue::AtomicUint32 value( 0 );
  value.Store( 10, Blue::MemoryOrder::Release );

  EXPECT_EQ( value.Load( Blue::MemoryOrder::Acquire ), 10u );

  Blue::Uint32 expected = 10;
  ASSERT_TRUE( value.CompareExchange( expected, 20, Blue::MemoryOrder::AcquireRelease ) );
  EXPECT_EQ( expected, 10u );
  EXPECT_EQ( value.Load( Blue::MemoryOrder::Acquire ), 20u );
}

TEST( BlueSystemPhase2Tests, SpinLockCanBeAcquiredAndReleased )
{
  Blue::SpinLock lock;

  ASSERT_TRUE( lock.TryAcquire( ) );
  lock.Release( );
}

TEST( BlueSystemPhase2Tests, NativeThreadRunsAndJoinsWithExitCode )
{
  TestState state = { };
  state.Counter.Store( 0, Blue::MemoryOrder::Relaxed );

  Blue::Thread thread = { };
  Blue::ThreadCreateDesc threadDesc = { };
  threadDesc.Name = "BlueSystemTest";
  threadDesc.Entry = ThreadEntry;
  threadDesc.UserData = &state;

  ASSERT_TRUE( Blue::CreateThread( thread, threadDesc ) );

  Blue::Uint32 exitCode = 0;
  ASSERT_TRUE( Blue::JoinThread( thread, &exitCode ) );

  EXPECT_EQ( exitCode, 7u );
  EXPECT_EQ( state.Counter.Load( Blue::MemoryOrder::Acquire ), 10000u );
}

TEST( BlueSystemPhase2Tests, NativeThreadCanBeDetached )
{
  Blue::Thread detached = { };
  Blue::ThreadCreateDesc detachDesc = { };
  detachDesc.Name = "BlueDetachedTest";
  detachDesc.Entry = DetachedThreadEntry;

  ASSERT_TRUE( Blue::CreateThread( detached, detachDesc ) );
  ASSERT_TRUE( Blue::DetachThread( detached ) );

  Blue::SleepCurrentThread( 10 );
}

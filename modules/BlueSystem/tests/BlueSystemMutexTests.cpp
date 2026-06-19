#include <Blue/System/Mutex.h>
#include <Blue/System/Thread.h>
#include <Blue/System/Types.h>

#include <gtest/gtest.h>


namespace
{
struct MutexIncrementContext final
{
  Blue::Mutex* Mutex;
  Blue::Uint32* Counter;
  Blue::Uint32 Iterations;
};

Blue::Uint32 MutexIncrementThreadEntry( void* userData )
{
  MutexIncrementContext* context = static_cast< MutexIncrementContext* >( userData );
  Blue::SetCurrentThreadName( "BlueMutexWorker" );

  for ( Blue::Uint32 index = 0; index < context->Iterations; ++index )
  {
    Blue::ScopedMutexLock lock( *context->Mutex );
    ++( *context->Counter );
  }

  return 0;
}
} // namespace

static void TestMutexLifecycle( )
{
  Blue::Mutex mutex;

  ASSERT_TRUE( !Blue::IsMutexInitialized( mutex ) );
  ASSERT_TRUE( Blue::InitializeMutex( mutex ) );
  ASSERT_TRUE( Blue::IsMutexInitialized( mutex ) );

  Blue::AcquireMutex( mutex );
  Blue::ReleaseMutex( mutex );

  mutex.Acquire( );
  mutex.Release( );

  ASSERT_TRUE( Blue::TryAcquireMutex( mutex ) );
  Blue::ReleaseMutex( mutex );

  ASSERT_TRUE( mutex.TryAcquire( ) );
  mutex.Release( );

  Blue::ShutdownMutex( mutex );
  ASSERT_TRUE( !Blue::IsMutexInitialized( mutex ) );
}

static void TestScopedMutexLock( )
{
  Blue::Mutex mutex;
  ASSERT_TRUE( Blue::InitializeMutex( mutex ) );

  Blue::Uint32 value = 0;

  {
    Blue::ScopedMutexLock lock( mutex );
    ++value;
  }

  ASSERT_TRUE( value == 1 );
  ASSERT_TRUE( Blue::TryAcquireMutex( mutex ) );
  Blue::ReleaseMutex( mutex );

  Blue::ShutdownMutex( mutex );
}

static void TestOwnedMutexLifecycle( )
{
  Blue::OwnedMutex mutex;
  ASSERT_TRUE( mutex.IsValid( ) );

  mutex.Acquire( );
  mutex.Release( );

  ASSERT_TRUE( mutex.TryAcquire( ) );
  mutex.Release( );
}

static void TestMutexWithMultipleThreads( )
{
  constexpr Blue::Uint32 ThreadCount = 8;
  constexpr Blue::Uint32 Iterations = 20000;

  Blue::Mutex mutex;
  ASSERT_TRUE( Blue::InitializeMutex( mutex ) );

  Blue::Uint32 counter = 0;
  Blue::Thread threads[ ThreadCount ];
  MutexIncrementContext contexts[ ThreadCount ];

  for ( Blue::Uint32 index = 0; index < ThreadCount; ++index )
  {
    contexts[ index ].Mutex = &mutex;
    contexts[ index ].Counter = &counter;
    contexts[ index ].Iterations = Iterations;

    Blue::ThreadCreateDesc desc;
    desc.Name = "BlueMutexWorker";
    desc.Entry = &MutexIncrementThreadEntry;
    desc.UserData = &contexts[ index ];

    ASSERT_TRUE( Blue::CreateThread( threads[ index ], desc ) );
  }

  for ( Blue::Uint32 index = 0; index < ThreadCount; ++index )
  {
    ASSERT_TRUE( Blue::JoinThread( threads[ index ] ) );
  }

  ASSERT_TRUE( counter == ThreadCount * Iterations );
  Blue::ShutdownMutex( mutex );
}

TEST( BlueSystemMutexTests, RunsSuccessfully )
{
  TestMutexLifecycle( );
  TestScopedMutexLock( );
  TestOwnedMutexLifecycle( );
  TestMutexWithMultipleThreads( );
}

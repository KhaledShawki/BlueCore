#include <Blue/System/Atomic.h>
#include <Blue/System/Semaphore.h>
#include <Blue/System/Thread.h>
#include <Blue/System/Types.h>

#include <gtest/gtest.h>


namespace
{
struct SemaphoreWorkerContext final
{
  Blue::Semaphore* Semaphore;
  Blue::AtomicUint32* Counter;
};

Blue::Uint32 SemaphoreWorkerEntry( void* userData )
{
  SemaphoreWorkerContext* context = static_cast< SemaphoreWorkerContext* >( userData );
  Blue::SetCurrentThreadName( "BlueSemaphoreWorker" );
  context->Semaphore->Acquire( );
  context->Counter->FetchAdd( 1, Blue::MemoryOrder::Release );
  return 0;
}
} // namespace

static void TestSemaphoreLifecycle( )
{
  Blue::Semaphore semaphore;
  ASSERT_TRUE( !Blue::IsSemaphoreInitialized( semaphore ) );

  Blue::SemaphoreCreateDesc desc;
  desc.InitialCount = 0;
  desc.MaximumCount = 2;

  ASSERT_TRUE( Blue::InitializeSemaphore( semaphore, desc ) );
  ASSERT_TRUE( Blue::IsSemaphoreInitialized( semaphore ) );
  ASSERT_TRUE( !Blue::TryAcquireSemaphore( semaphore ) );

  ASSERT_TRUE( Blue::ReleaseSemaphore( semaphore ) );
  ASSERT_TRUE( Blue::TryAcquireSemaphore( semaphore ) );
  ASSERT_TRUE( !Blue::TryAcquireSemaphore( semaphore ) );

  ASSERT_TRUE( semaphore.Release( 2 ) );
  ASSERT_TRUE( semaphore.TryAcquire( ) );
  ASSERT_TRUE( semaphore.TryAcquire( ) );
  ASSERT_TRUE( !semaphore.TryAcquire( ) );

  Blue::ShutdownSemaphore( semaphore );
  ASSERT_TRUE( !Blue::IsSemaphoreInitialized( semaphore ) );
}

static void TestOwnedSemaphoreAndTimedAcquire( )
{
  Blue::SemaphoreCreateDesc desc;
  desc.InitialCount = 0;
  desc.MaximumCount = 1;

  Blue::OwnedSemaphore semaphore( desc );
  ASSERT_TRUE( semaphore.IsValid( ) );
  ASSERT_TRUE( !semaphore.AcquireFor( Blue::MakeTimeDurationFromMilliseconds( 5 ) ) );

  ASSERT_TRUE( semaphore.Release( ) );
  ASSERT_TRUE( semaphore.AcquireFor( Blue::MakeTimeDurationFromMilliseconds( 50 ) ) );
  ASSERT_TRUE( !semaphore.TryAcquire( ) );
}

static void TestSemaphoreWithThreads( )
{
  constexpr Blue::Uint32 ThreadCount = 8;

  Blue::Semaphore semaphore;
  Blue::SemaphoreCreateDesc desc;
  desc.InitialCount = 0;
  desc.MaximumCount = ThreadCount;
  ASSERT_TRUE( Blue::InitializeSemaphore( semaphore, desc ) );

  Blue::AtomicUint32 counter( 0 );
  Blue::Thread threads[ ThreadCount ];
  SemaphoreWorkerContext contexts[ ThreadCount ];

  for ( Blue::Uint32 index = 0; index < ThreadCount; ++index )
  {
    contexts[ index ].Semaphore = &semaphore;
    contexts[ index ].Counter = &counter;

    Blue::ThreadCreateDesc threadDesc;
    threadDesc.Name = "BlueSemaphoreWorker";
    threadDesc.Entry = &SemaphoreWorkerEntry;
    threadDesc.UserData = &contexts[ index ];

    ASSERT_TRUE( Blue::CreateThread( threads[ index ], threadDesc ) );
  }

  for ( Blue::Uint32 attempt = 0; attempt < 100; ++attempt )
  {
    Blue::YieldThread( );
  }

  ASSERT_TRUE( counter.Load( Blue::MemoryOrder::Acquire ) == 0 );
  ASSERT_TRUE( semaphore.Release( ThreadCount ) );

  for ( Blue::Uint32 index = 0; index < ThreadCount; ++index )
  {
    ASSERT_TRUE( Blue::JoinThread( threads[ index ] ) );
  }

  ASSERT_TRUE( counter.Load( Blue::MemoryOrder::Acquire ) == ThreadCount );
  Blue::ShutdownSemaphore( semaphore );
}

TEST( BlueSystemSemaphoreTests, RunsSuccessfully )
{
  TestSemaphoreLifecycle( );
  TestOwnedSemaphoreAndTimedAcquire( );
  TestSemaphoreWithThreads( );
}

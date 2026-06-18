#include <Blue/System/Atomic.h>
#include <Blue/System/Semaphore.h>
#include <Blue/System/Thread.h>
#include <Blue/System/Types.h>

#include <stdio.h>
#include <stdlib.h>

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
  BLUE_TEST_EXPECT( !Blue::IsSemaphoreInitialized( semaphore ) );

  Blue::SemaphoreCreateDesc desc;
  desc.InitialCount = 0;
  desc.MaximumCount = 2;

  BLUE_TEST_EXPECT( Blue::InitializeSemaphore( semaphore, desc ) );
  BLUE_TEST_EXPECT( Blue::IsSemaphoreInitialized( semaphore ) );
  BLUE_TEST_EXPECT( !Blue::TryAcquireSemaphore( semaphore ) );

  BLUE_TEST_EXPECT( Blue::ReleaseSemaphore( semaphore ) );
  BLUE_TEST_EXPECT( Blue::TryAcquireSemaphore( semaphore ) );
  BLUE_TEST_EXPECT( !Blue::TryAcquireSemaphore( semaphore ) );

  BLUE_TEST_EXPECT( semaphore.Release( 2 ) );
  BLUE_TEST_EXPECT( semaphore.TryAcquire( ) );
  BLUE_TEST_EXPECT( semaphore.TryAcquire( ) );
  BLUE_TEST_EXPECT( !semaphore.TryAcquire( ) );

  Blue::ShutdownSemaphore( semaphore );
  BLUE_TEST_EXPECT( !Blue::IsSemaphoreInitialized( semaphore ) );
}

static void TestOwnedSemaphoreAndTimedAcquire( )
{
  Blue::SemaphoreCreateDesc desc;
  desc.InitialCount = 0;
  desc.MaximumCount = 1;

  Blue::OwnedSemaphore semaphore( desc );
  BLUE_TEST_EXPECT( semaphore.IsValid( ) );
  BLUE_TEST_EXPECT( !semaphore.AcquireFor( Blue::MakeTimeDurationFromMilliseconds( 5 ) ) );

  BLUE_TEST_EXPECT( semaphore.Release( ) );
  BLUE_TEST_EXPECT( semaphore.AcquireFor( Blue::MakeTimeDurationFromMilliseconds( 50 ) ) );
  BLUE_TEST_EXPECT( !semaphore.TryAcquire( ) );
}

static void TestSemaphoreWithThreads( )
{
  constexpr Blue::Uint32 ThreadCount = 8;

  Blue::Semaphore semaphore;
  Blue::SemaphoreCreateDesc desc;
  desc.InitialCount = 0;
  desc.MaximumCount = ThreadCount;
  BLUE_TEST_EXPECT( Blue::InitializeSemaphore( semaphore, desc ) );

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

    BLUE_TEST_EXPECT( Blue::CreateThread( threads[ index ], threadDesc ) );
  }

  for ( Blue::Uint32 attempt = 0; attempt < 100; ++attempt )
  {
    Blue::YieldThread( );
  }

  BLUE_TEST_EXPECT( counter.Load( Blue::MemoryOrder::Acquire ) == 0 );
  BLUE_TEST_EXPECT( semaphore.Release( ThreadCount ) );

  for ( Blue::Uint32 index = 0; index < ThreadCount; ++index )
  {
    BLUE_TEST_EXPECT( Blue::JoinThread( threads[ index ] ) );
  }

  BLUE_TEST_EXPECT( counter.Load( Blue::MemoryOrder::Acquire ) == ThreadCount );
  Blue::ShutdownSemaphore( semaphore );
}

int main( )
{
  TestSemaphoreLifecycle( );
  TestOwnedSemaphoreAndTimedAcquire( );
  TestSemaphoreWithThreads( );

  printf( "BlueSystem semaphore tests passed.\n" );
  return 0;
}

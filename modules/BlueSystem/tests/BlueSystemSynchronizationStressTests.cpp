#include <Blue/System/Atomic.h>
#include <Blue/System/ConditionVariable.h>
#include <Blue/System/Event.h>
#include <Blue/System/Mutex.h>
#include <Blue/System/Semaphore.h>
#include <Blue/System/Thread.h>
#include <Blue/System/Types.h>

#include <gtest/gtest.h>


namespace
{
struct StressContext final
{
  Blue::Semaphore* WorkSemaphore;
  Blue::Event* StartEvent;
  Blue::ConditionVariable* DoneCondition;
  Blue::Mutex* Mutex;
  Blue::AtomicUint32* ConsumedCount;
  Blue::Uint32* DoneCount;
  Blue::Uint32 WorkPerThread;
};

Blue::Uint32 StressWorkerEntry( void* userData )
{
  StressContext* context = static_cast< StressContext* >( userData );
  Blue::SetCurrentThreadName( "BlueSyncStressWorker" );

  context->StartEvent->Wait( );

  for ( Blue::Uint32 index = 0; index < context->WorkPerThread; ++index )
  {
    context->WorkSemaphore->Acquire( );
    context->ConsumedCount->FetchAdd( 1, Blue::MemoryOrder::Relaxed );
  }

  context->Mutex->Acquire( );
  ++( *context->DoneCount );
  context->DoneCondition->NotifyAll( );
  context->Mutex->Release( );
  return 0;
}
} // namespace

static void TestSynchronizationStress( )
{
  constexpr Blue::Uint32 ThreadCount = 8;
  constexpr Blue::Uint32 WorkPerThread = 1000;
  constexpr Blue::Uint32 TotalWork = ThreadCount * WorkPerThread;

  Blue::Semaphore workSemaphore;
  Blue::Event startEvent;
  Blue::ConditionVariable doneCondition;
  Blue::Mutex mutex;

  Blue::SemaphoreCreateDesc semaphoreDesc;
  semaphoreDesc.InitialCount = 0;
  semaphoreDesc.MaximumCount = TotalWork;
  ASSERT_TRUE( Blue::InitializeSemaphore( workSemaphore, semaphoreDesc ) );

  Blue::EventCreateDesc eventDesc;
  eventDesc.ResetMode = Blue::EventResetMode::Manual;
  eventDesc.InitiallySignaled = false;
  ASSERT_TRUE( Blue::InitializeEvent( startEvent, eventDesc ) );

  ASSERT_TRUE( Blue::InitializeConditionVariable( doneCondition ) );
  ASSERT_TRUE( Blue::InitializeMutex( mutex ) );

  Blue::AtomicUint32 consumedCount( 0 );
  Blue::Uint32 doneCount = 0;
  Blue::Thread threads[ ThreadCount ];

  StressContext context;
  context.WorkSemaphore = &workSemaphore;
  context.StartEvent = &startEvent;
  context.DoneCondition = &doneCondition;
  context.Mutex = &mutex;
  context.ConsumedCount = &consumedCount;
  context.DoneCount = &doneCount;
  context.WorkPerThread = WorkPerThread;

  for ( Blue::Uint32 index = 0; index < ThreadCount; ++index )
  {
    Blue::ThreadCreateDesc desc;
    desc.Name = "BlueSyncStressWorker";
    desc.Entry = &StressWorkerEntry;
    desc.UserData = &context;
    ASSERT_TRUE( Blue::CreateThread( threads[ index ], desc ) );
  }

  ASSERT_TRUE( workSemaphore.Release( TotalWork ) );
  startEvent.Signal( );

  mutex.Acquire( );
  while ( doneCount != ThreadCount )
  {
    doneCondition.Wait( mutex );
  }
  mutex.Release( );

  for ( Blue::Uint32 index = 0; index < ThreadCount; ++index )
  {
    ASSERT_TRUE( Blue::JoinThread( threads[ index ] ) );
  }

  ASSERT_TRUE( consumedCount.Load( Blue::MemoryOrder::Acquire ) == TotalWork );

  Blue::ShutdownMutex( mutex );
  Blue::ShutdownConditionVariable( doneCondition );
  Blue::ShutdownEvent( startEvent );
  Blue::ShutdownSemaphore( workSemaphore );
}

TEST( BlueSystemSynchronizationStressTests, RunsSuccessfully )
{
  TestSynchronizationStress( );
}

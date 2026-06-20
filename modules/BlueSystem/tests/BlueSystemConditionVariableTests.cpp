#include <Blue/System/Atomic.h>
#include <Blue/System/ConditionVariable.h>
#include <Blue/System/Mutex.h>
#include <Blue/System/Thread.h>
#include <Blue/System/Types.h>

#include "BlueSystemTestWait.h"

#include <gtest/gtest.h>

namespace
{
static void JoinCreatedThreads( Blue::Thread* threads, Blue::Uint32 threadCount )
{
  for ( Blue::Uint32 index = 0; index < threadCount; ++index )
  {
    if ( Blue::IsThreadJoinable( threads[ index ] ) )
    {
      Blue::JoinThread( threads[ index ] );
    }
  }
}

struct ConditionVariableContext final
{
  Blue::ConditionVariable* ConditionVariable = nullptr;
  Blue::Mutex* Mutex = nullptr;
  Blue::AtomicUint32* WaitingCount = nullptr;
  Blue::Uint32* Ready = nullptr;
  Blue::Uint32* Completed = nullptr;
};

Blue::Uint32 ConditionVariableWorkerEntry( void* userData )
{
  ConditionVariableContext* context = static_cast< ConditionVariableContext* >( userData );
  Blue::SetCurrentThreadName( "BlueConditionWorker" );

  context->Mutex->Acquire( );
  context->WaitingCount->FetchAdd( 1, Blue::MemoryOrder::Release );

  while ( *context->Ready == 0 )
  {
    context->ConditionVariable->Wait( *context->Mutex );
  }

  ++( *context->Completed );
  context->Mutex->Release( );

  return 0;
}

static void ReleaseConditionWorkers( Blue::Mutex& mutex,
                                     Blue::ConditionVariable& conditionVariable,
                                     Blue::Uint32& ready )
{
  mutex.Acquire( );
  ready = 1;
  conditionVariable.NotifyAll( );
  mutex.Release( );
}
} // namespace

TEST( BlueSystemConditionVariableTests, LifecycleInitializesNotifiesAndShutsDown )
{
  Blue::ConditionVariable conditionVariable;

  EXPECT_FALSE( Blue::IsConditionVariableInitialized( conditionVariable ) );

  ASSERT_TRUE( Blue::InitializeConditionVariable( conditionVariable ) );
  EXPECT_TRUE( Blue::IsConditionVariableInitialized( conditionVariable ) );

  Blue::NotifyOneConditionVariable( conditionVariable );
  Blue::NotifyAllConditionVariable( conditionVariable );

  Blue::ShutdownConditionVariable( conditionVariable );

  EXPECT_FALSE( Blue::IsConditionVariableInitialized( conditionVariable ) );
}

TEST( BlueSystemConditionVariableTests, OwnedConditionVariableTimedWaitTimesOut )
{
  Blue::OwnedMutex mutex;
  Blue::OwnedConditionVariable conditionVariable;

  ASSERT_TRUE( mutex.IsValid( ) );
  ASSERT_TRUE( conditionVariable.IsValid( ) );

  mutex.Acquire( );
  const Blue::Bool wasSignaled = conditionVariable.WaitFor( mutex.Get( ), Blue::MakeTimeDurationFromMilliseconds( 5 ) );
  mutex.Release( );

  EXPECT_FALSE( wasSignaled );
}

TEST( BlueSystemConditionVariableTests, NotifyAllReleasesAllWaitingWorkers )
{
  constexpr Blue::Uint32 ThreadCount = 6;

  Blue::OwnedMutex mutex;
  Blue::OwnedConditionVariable conditionVariable;

  ASSERT_TRUE( mutex.IsValid( ) );
  ASSERT_TRUE( conditionVariable.IsValid( ) );

  Blue::AtomicUint32 waitingCount( 0 );
  Blue::Uint32 ready = 0;
  Blue::Uint32 completed = 0;
  Blue::Thread threads[ ThreadCount ];

  ConditionVariableContext context;
  context.ConditionVariable = &conditionVariable.Get( );
  context.Mutex = &mutex.Get( );
  context.WaitingCount = &waitingCount;
  context.Ready = &ready;
  context.Completed = &completed;

  Blue::Uint32 createdThreadCount = 0;

  for ( Blue::Uint32 index = 0; index < ThreadCount; ++index )
  {
    Blue::ThreadCreateDesc desc;
    desc.Name = "BlueConditionWorker";
    desc.Entry = &ConditionVariableWorkerEntry;
    desc.UserData = &context;

    if ( !Blue::CreateThread( threads[ index ], desc ) )
    {
      ReleaseConditionWorkers( mutex.Get( ), conditionVariable.Get( ), ready );
      JoinCreatedThreads( threads, createdThreadCount );
      FAIL( ) << "Failed to create condition-variable worker thread " << index << ".";
      return;
    }

    ++createdThreadCount;
  }

  if ( !BlueSystemTest::WaitUntil(
         [ &waitingCount ]( )
         {
           return waitingCount.Load( Blue::MemoryOrder::Acquire ) == ThreadCount;
         } ) )
  {
    ReleaseConditionWorkers( mutex.Get( ), conditionVariable.Get( ), ready );
    JoinCreatedThreads( threads, createdThreadCount );
    FAIL( ) << "Condition-variable workers did not enter the wait state before timeout.";
    return;
  }

  ReleaseConditionWorkers( mutex.Get( ), conditionVariable.Get( ), ready );

  for ( Blue::Uint32 index = 0; index < createdThreadCount; ++index )
  {
    EXPECT_TRUE( Blue::JoinThread( threads[ index ] ) );
  }

  EXPECT_EQ( completed, ThreadCount );
}

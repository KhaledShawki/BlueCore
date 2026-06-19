#include <Blue/System/Atomic.h>
#include <Blue/System/SpinLock.h>
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

struct ThreadIncrementContext final
{
  Blue::AtomicUint32* Counter = nullptr;
  Blue::Uint32 Iterations = 0;
  Blue::Uint32 ExitCode = 0;
};

struct SpinLockThreadContext final
{
  Blue::SpinLock* Lock = nullptr;
  Blue::Uint32* Counter = nullptr;
  Blue::Uint32 Iterations = 0;
};

struct ThreadPolicyContext final
{
  Blue::AtomicUint32 Started{ 0 };
  Blue::AtomicUint32 Release{ 0 };
};

Blue::Uint32 IncrementThreadEntry( void* userData )
{
  ThreadIncrementContext* context = static_cast< ThreadIncrementContext* >( userData );
  Blue::SetCurrentThreadName( "BlueIncrementWorker" );

  for ( Blue::Uint32 index = 0; index < context->Iterations; ++index )
  {
    context->Counter->FetchAdd( 1, Blue::MemoryOrder::Relaxed );
  }

  return context->ExitCode;
}

Blue::Uint32 SpinLockThreadEntry( void* userData )
{
  SpinLockThreadContext* context = static_cast< SpinLockThreadContext* >( userData );
  Blue::SetCurrentThreadName( "BlueSpinWorker" );

  for ( Blue::Uint32 index = 0; index < context->Iterations; ++index )
  {
    Blue::ScopedSpinLock lock( *context->Lock );
    ++( *context->Counter );
  }

  return 0;
}

Blue::Uint32 DetachThreadEntry( void* userData )
{
  Blue::AtomicUint32* flag = static_cast< Blue::AtomicUint32* >( userData );
  Blue::SetCurrentThreadName( "BlueDetachWorker" );
  flag->Store( 1, Blue::MemoryOrder::Release );
  return 0;
}

Blue::Uint32 OwnedThreadEntry( void* userData )
{
  Blue::AtomicUint32* flag = static_cast< Blue::AtomicUint32* >( userData );
  Blue::SetCurrentThreadName( "BlueOwnedWorker" );
  flag->Store( 1, Blue::MemoryOrder::Release );
  return 23;
}

Blue::Uint32 PolicyThreadEntry( void* userData )
{
  ThreadPolicyContext* context = static_cast< ThreadPolicyContext* >( userData );
  Blue::SetCurrentThreadName( "BluePolicyWorker" );

  BLUE_UNUSED( Blue::SetCurrentThreadPriority( Blue::ThreadPriority::Normal ) );
  BLUE_UNUSED( Blue::SetCurrentThreadAffinity( Blue::CpuAffinity::Any( ) ) );

  context->Started.Store( 1, Blue::MemoryOrder::Release );

  while ( context->Release.Load( Blue::MemoryOrder::Acquire ) == 0 )
  {
    Blue::YieldThread( );
    Blue::SleepCurrentThread( 1 );
  }

  return 0;
}

static void ReleaseAndJoinPolicyThread( Blue::Thread& thread, ThreadPolicyContext& context )
{
  context.Release.Store( 1, Blue::MemoryOrder::Release );

  if ( Blue::IsThreadJoinable( thread ) )
  {
    Blue::JoinThread( thread );
  }
}
} // namespace

TEST( BlueSystemThreadingTests, CurrentThreadUtilitiesAreCallable )
{
  const Blue::ThreadId currentThreadId = Blue::GetCurrentThreadId( );

  EXPECT_NE( currentThreadId, Blue::ThreadId{ 0 } );

  Blue::SetCurrentThreadName( "BlueMainTest" );
  Blue::YieldThread( );
  Blue::SleepCurrentThread( 1 );
}

TEST( BlueSystemThreadingTests, CpuAffinityHelpersExposeExpectedMasks )
{
  constexpr Blue::CpuAffinity any = Blue::CpuAffinity::Any( );
  constexpr Blue::CpuAffinity first = Blue::CpuAffinity::FromProcessorIndex( 0 );
  constexpr Blue::CpuAffinity invalid = Blue::CpuAffinity::FromProcessorIndex( 64 );

  EXPECT_FALSE( any.IsEnabled( ) );
  EXPECT_TRUE( first.IsEnabled( ) );
  EXPECT_TRUE( first.ContainsProcessor( 0 ) );
  EXPECT_FALSE( first.ContainsProcessor( 1 ) );
  EXPECT_FALSE( invalid.IsEnabled( ) );
}

TEST( BlueSystemThreadingTests, CreateJoinRunsWorkerAndReturnsExitCode )
{
  Blue::AtomicUint32 counter( 0 );

  ThreadIncrementContext context;
  context.Counter = &counter;
  context.Iterations = 10000;
  context.ExitCode = 17;

  Blue::Thread thread;

  Blue::ThreadCreateDesc desc;
  desc.Name = "BlueJoinWorker";
  desc.Entry = &IncrementThreadEntry;
  desc.UserData = &context;
  desc.StackSize = 0;
  desc.Priority = Blue::ThreadPriority::Normal;

  ASSERT_TRUE( Blue::CreateThread( thread, desc ) );
  EXPECT_TRUE( Blue::IsThreadJoinable( thread ) );
  EXPECT_NE( thread.Id, Blue::ThreadId{ 0 } );

  Blue::Uint32 exitCode = 0;
  ASSERT_TRUE( Blue::JoinThread( thread, &exitCode ) );

  EXPECT_EQ( exitCode, context.ExitCode );
  EXPECT_FALSE( Blue::IsThreadJoinable( thread ) );
  EXPECT_EQ( counter.Load( Blue::MemoryOrder::Acquire ), context.Iterations );
}

TEST( BlueSystemThreadingTests, DetachReleasesJoinHandleAndRunsWorker )
{
  Blue::AtomicUint32 flag( 0 );

  Blue::Thread thread;

  Blue::ThreadCreateDesc desc;
  desc.Name = "BlueDetachWorker";
  desc.Entry = &DetachThreadEntry;
  desc.UserData = &flag;

  ASSERT_TRUE( Blue::CreateThread( thread, desc ) );
  EXPECT_TRUE( Blue::IsThreadJoinable( thread ) );

  ASSERT_TRUE( Blue::DetachThread( thread ) );
  EXPECT_FALSE( Blue::IsThreadJoinable( thread ) );

  EXPECT_TRUE( BlueSystemTest::WaitUntil(
    [ &flag ]( )
    {
      return flag.Load( Blue::MemoryOrder::Acquire ) == 1;
    } ) )
    << "Detached worker did not publish completion before timeout.";
}

TEST( BlueSystemThreadingTests, OwnedThreadJoinLifecycleRunsWorkerAndReturnsExitCode )
{
  Blue::AtomicUint32 flag( 0 );

  Blue::ThreadCreateInfo createInfo;
  createInfo.Name = "BlueOwnedWorker";
  createInfo.Entry = &OwnedThreadEntry;
  createInfo.UserData = &flag;

  Blue::OwnedThread thread( createInfo );

  ASSERT_TRUE( thread.IsJoinable( ) );
  EXPECT_NE( thread.GetId( ), Blue::ThreadId{ 0 } );

  Blue::Uint32 exitCode = 0;
  ASSERT_TRUE( thread.Join( &exitCode ) );

  EXPECT_EQ( exitCode, 23u );
  EXPECT_FALSE( thread.IsJoinable( ) );
  EXPECT_EQ( flag.Load( Blue::MemoryOrder::Acquire ), 1u );
}

TEST( BlueSystemThreadingTests, OwnedThreadDetachLifecycleRunsWorker )
{
  Blue::AtomicUint32 flag( 0 );
  Blue::OwnedThread thread;

  Blue::ThreadCreateInfo createInfo;
  createInfo.Name = "BlueOwnedDetach";
  createInfo.Entry = &DetachThreadEntry;
  createInfo.UserData = &flag;

  ASSERT_TRUE( thread.Create( createInfo ) );
  EXPECT_TRUE( thread.IsJoinable( ) );

  ASSERT_TRUE( thread.Detach( ) );
  EXPECT_FALSE( thread.IsJoinable( ) );

  EXPECT_TRUE( BlueSystemTest::WaitUntil(
    [ &flag ]( )
    {
      return flag.Load( Blue::MemoryOrder::Acquire ) == 1;
    } ) )
    << "Owned detached worker did not publish completion before timeout.";
}

TEST( BlueSystemThreadingTests, ThreadPolicyApiSmokeAppliesToLiveThread )
{
  ThreadPolicyContext context;

  Blue::Thread thread;

  Blue::ThreadCreateInfo createInfo;
  createInfo.Name = "BluePolicyWorker";
  createInfo.Entry = &PolicyThreadEntry;
  createInfo.UserData = &context;
  createInfo.Priority = Blue::ThreadPriority::Normal;
  createInfo.Affinity = Blue::CpuAffinity::Any( );

  ASSERT_TRUE( Blue::CreateThread( thread, createInfo ) );

  if ( !BlueSystemTest::WaitUntil(
         [ &context ]( )
         {
           return context.Started.Load( Blue::MemoryOrder::Acquire ) == 1;
         } ) )
  {
    ReleaseAndJoinPolicyThread( thread, context );
    FAIL( ) << "Policy worker did not start before timeout.";
    return;
  }

  EXPECT_TRUE( Blue::SetThreadAffinity( thread, Blue::CpuAffinity::Any( ) ) );
  BLUE_UNUSED( Blue::SetThreadPriority( thread, Blue::ThreadPriority::Normal ) );

  const Blue::CpuAffinity firstProcessor = Blue::CpuAffinity::FromProcessorIndex( 0 );
  BLUE_UNUSED( Blue::SetThreadAffinity( thread, firstProcessor ) );

  context.Release.Store( 1, Blue::MemoryOrder::Release );

  ASSERT_TRUE( Blue::JoinThread( thread ) );

  EXPECT_TRUE( Blue::SetCurrentThreadAffinity( Blue::CpuAffinity::Any( ) ) );
  BLUE_UNUSED( Blue::SetCurrentThreadAffinity( firstProcessor ) );
  BLUE_UNUSED( Blue::SetCurrentThreadPriority( Blue::ThreadPriority::Normal ) );
}

TEST( BlueSystemThreadingTests, MultipleThreadsIncrementAtomicCounter )
{
  constexpr Blue::Uint32 ThreadCount = 8;
  constexpr Blue::Uint32 Iterations = 25000;

  Blue::AtomicUint32 counter( 0 );
  Blue::Thread threads[ ThreadCount ];
  ThreadIncrementContext contexts[ ThreadCount ];

  Blue::Uint32 createdThreadCount = 0;

  for ( Blue::Uint32 index = 0; index < ThreadCount; ++index )
  {
    contexts[ index ].Counter = &counter;
    contexts[ index ].Iterations = Iterations;
    contexts[ index ].ExitCode = index;

    Blue::ThreadCreateDesc desc;
    desc.Name = "BlueAtomicWorker";
    desc.Entry = &IncrementThreadEntry;
    desc.UserData = &contexts[ index ];

    if ( !Blue::CreateThread( threads[ index ], desc ) )
    {
      JoinCreatedThreads( threads, createdThreadCount );
      FAIL( ) << "Failed to create atomic worker thread " << index << ".";
      return;
    }

    ++createdThreadCount;
  }

  for ( Blue::Uint32 index = 0; index < createdThreadCount; ++index )
  {
    Blue::Uint32 exitCode = 0;
    const Blue::Bool joined = Blue::JoinThread( threads[ index ], &exitCode );

    EXPECT_TRUE( joined );

    if ( joined )
    {
      EXPECT_EQ( exitCode, index );
    }
  }

  EXPECT_EQ( counter.Load( Blue::MemoryOrder::Acquire ), ThreadCount * Iterations );
}

TEST( BlueSystemThreadingTests, MultipleThreadsSynchronizeWithSpinLock )
{
  constexpr Blue::Uint32 ThreadCount = 6;
  constexpr Blue::Uint32 Iterations = 10000;

  Blue::SpinLock lock;
  Blue::Uint32 counter = 0;
  Blue::Thread threads[ ThreadCount ];
  SpinLockThreadContext contexts[ ThreadCount ];

  Blue::Uint32 createdThreadCount = 0;

  for ( Blue::Uint32 index = 0; index < ThreadCount; ++index )
  {
    contexts[ index ].Lock = &lock;
    contexts[ index ].Counter = &counter;
    contexts[ index ].Iterations = Iterations;

    Blue::ThreadCreateDesc desc;
    desc.Name = "BlueSpinWorker";
    desc.Entry = &SpinLockThreadEntry;
    desc.UserData = &contexts[ index ];

    if ( !Blue::CreateThread( threads[ index ], desc ) )
    {
      JoinCreatedThreads( threads, createdThreadCount );
      FAIL( ) << "Failed to create spin-lock worker thread " << index << ".";
      return;
    }

    ++createdThreadCount;
  }

  for ( Blue::Uint32 index = 0; index < createdThreadCount; ++index )
  {
    EXPECT_TRUE( Blue::JoinThread( threads[ index ] ) );
  }

  EXPECT_EQ( counter, ThreadCount * Iterations );
}

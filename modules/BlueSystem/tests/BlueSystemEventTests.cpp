#include <Blue/System/Atomic.h>
#include <Blue/System/Event.h>
#include <Blue/System/Thread.h>
#include <Blue/System/Types.h>

#include <gtest/gtest.h>


namespace
{
struct EventWorkerContext final
{
  Blue::Event* Event;
  Blue::AtomicUint32* Flag;
};

Blue::Uint32 EventWorkerEntry( void* userData )
{
  EventWorkerContext* context = static_cast< EventWorkerContext* >( userData );
  Blue::SetCurrentThreadName( "BlueEventWorker" );
  context->Event->Wait( );
  context->Flag->Store( 1, Blue::MemoryOrder::Release );
  return 0;
}
} // namespace

static void TestManualResetEventLifecycle( )
{
  Blue::Event event;
  Blue::EventCreateDesc desc;
  desc.ResetMode = Blue::EventResetMode::Manual;
  desc.InitiallySignaled = false;

  ASSERT_TRUE( !Blue::IsEventInitialized( event ) );
  ASSERT_TRUE( Blue::InitializeEvent( event, desc ) );
  ASSERT_TRUE( Blue::IsEventInitialized( event ) );
  ASSERT_TRUE( !Blue::TryWaitEvent( event ) );

  Blue::SignalEvent( event );
  ASSERT_TRUE( Blue::TryWaitEvent( event ) );
  ASSERT_TRUE( Blue::TryWaitEvent( event ) );

  Blue::ResetEvent( event );
  ASSERT_TRUE( !Blue::TryWaitEvent( event ) );

  Blue::ShutdownEvent( event );
  ASSERT_TRUE( !Blue::IsEventInitialized( event ) );
}

static void TestAutoResetEventLifecycle( )
{
  Blue::Event event;
  Blue::EventCreateDesc desc;
  desc.ResetMode = Blue::EventResetMode::Auto;
  desc.InitiallySignaled = false;

  ASSERT_TRUE( Blue::InitializeEvent( event, desc ) );
  ASSERT_TRUE( !event.TryWait( ) );

  event.Signal( );
  ASSERT_TRUE( event.TryWait( ) );
  ASSERT_TRUE( !event.TryWait( ) );

  Blue::ShutdownEvent( event );
}

static void TestOwnedEventAndTimedWait( )
{
  Blue::EventCreateDesc desc;
  desc.ResetMode = Blue::EventResetMode::Auto;
  desc.InitiallySignaled = false;

  Blue::OwnedEvent event( desc );
  ASSERT_TRUE( event.IsValid( ) );
  ASSERT_TRUE( !event.WaitFor( Blue::MakeTimeDurationFromMilliseconds( 5 ) ) );

  event.Signal( );
  ASSERT_TRUE( event.WaitFor( Blue::MakeTimeDurationFromMilliseconds( 50 ) ) );
  ASSERT_TRUE( !event.TryWait( ) );
}

static void TestEventWithThread( )
{
  Blue::Event event;
  Blue::EventCreateDesc desc;
  desc.ResetMode = Blue::EventResetMode::Manual;
  desc.InitiallySignaled = false;
  ASSERT_TRUE( Blue::InitializeEvent( event, desc ) );

  Blue::AtomicUint32 flag( 0 );
  EventWorkerContext context;
  context.Event = &event;
  context.Flag = &flag;

  Blue::Thread thread;
  Blue::ThreadCreateDesc threadDesc;
  threadDesc.Name = "BlueEventWorker";
  threadDesc.Entry = &EventWorkerEntry;
  threadDesc.UserData = &context;

  ASSERT_TRUE( Blue::CreateThread( thread, threadDesc ) );

  for ( Blue::Uint32 attempt = 0; attempt < 100; ++attempt )
  {
    Blue::YieldThread( );
  }

  ASSERT_TRUE( flag.Load( Blue::MemoryOrder::Acquire ) == 0 );
  event.Signal( );
  ASSERT_TRUE( Blue::JoinThread( thread ) );
  ASSERT_TRUE( flag.Load( Blue::MemoryOrder::Acquire ) == 1 );

  Blue::ShutdownEvent( event );
}

TEST( BlueSystemEventTests, RunsSuccessfully )
{
  TestManualResetEventLifecycle( );
  TestAutoResetEventLifecycle( );
  TestOwnedEventAndTimedWait( );
  TestEventWithThread( );
}

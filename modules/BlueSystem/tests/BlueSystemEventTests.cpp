#include <Blue/System/Atomic.h>
#include <Blue/System/Event.h>
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

  BLUE_TEST_EXPECT( !Blue::IsEventInitialized( event ) );
  BLUE_TEST_EXPECT( Blue::InitializeEvent( event, desc ) );
  BLUE_TEST_EXPECT( Blue::IsEventInitialized( event ) );
  BLUE_TEST_EXPECT( !Blue::TryWaitEvent( event ) );

  Blue::SignalEvent( event );
  BLUE_TEST_EXPECT( Blue::TryWaitEvent( event ) );
  BLUE_TEST_EXPECT( Blue::TryWaitEvent( event ) );

  Blue::ResetEvent( event );
  BLUE_TEST_EXPECT( !Blue::TryWaitEvent( event ) );

  Blue::ShutdownEvent( event );
  BLUE_TEST_EXPECT( !Blue::IsEventInitialized( event ) );
}

static void TestAutoResetEventLifecycle( )
{
  Blue::Event event;
  Blue::EventCreateDesc desc;
  desc.ResetMode = Blue::EventResetMode::Auto;
  desc.InitiallySignaled = false;

  BLUE_TEST_EXPECT( Blue::InitializeEvent( event, desc ) );
  BLUE_TEST_EXPECT( !event.TryWait( ) );

  event.Signal( );
  BLUE_TEST_EXPECT( event.TryWait( ) );
  BLUE_TEST_EXPECT( !event.TryWait( ) );

  Blue::ShutdownEvent( event );
}

static void TestOwnedEventAndTimedWait( )
{
  Blue::EventCreateDesc desc;
  desc.ResetMode = Blue::EventResetMode::Auto;
  desc.InitiallySignaled = false;

  Blue::OwnedEvent event( desc );
  BLUE_TEST_EXPECT( event.IsValid( ) );
  BLUE_TEST_EXPECT( !event.WaitFor( Blue::MakeTimeDurationFromMilliseconds( 5 ) ) );

  event.Signal( );
  BLUE_TEST_EXPECT( event.WaitFor( Blue::MakeTimeDurationFromMilliseconds( 50 ) ) );
  BLUE_TEST_EXPECT( !event.TryWait( ) );
}

static void TestEventWithThread( )
{
  Blue::Event event;
  Blue::EventCreateDesc desc;
  desc.ResetMode = Blue::EventResetMode::Manual;
  desc.InitiallySignaled = false;
  BLUE_TEST_EXPECT( Blue::InitializeEvent( event, desc ) );

  Blue::AtomicUint32 flag( 0 );
  EventWorkerContext context;
  context.Event = &event;
  context.Flag = &flag;

  Blue::Thread thread;
  Blue::ThreadCreateDesc threadDesc;
  threadDesc.Name = "BlueEventWorker";
  threadDesc.Entry = &EventWorkerEntry;
  threadDesc.UserData = &context;

  BLUE_TEST_EXPECT( Blue::CreateThread( thread, threadDesc ) );

  for ( Blue::Uint32 attempt = 0; attempt < 100; ++attempt )
  {
    Blue::YieldThread( );
  }

  BLUE_TEST_EXPECT( flag.Load( Blue::MemoryOrder::Acquire ) == 0 );
  event.Signal( );
  BLUE_TEST_EXPECT( Blue::JoinThread( thread ) );
  BLUE_TEST_EXPECT( flag.Load( Blue::MemoryOrder::Acquire ) == 1 );

  Blue::ShutdownEvent( event );
}

int main( )
{
  TestManualResetEventLifecycle( );
  TestAutoResetEventLifecycle( );
  TestOwnedEventAndTimedWait( );
  TestEventWithThread( );

  printf( "BlueSystem event tests passed.\n" );
  return 0;
}

#include <Blue/System/Log.h>
#include <Blue/System/Thread.h>
#include <Blue/System/Threading/Atomic.h>
#include <Blue/System/Types.h>

#include <gtest/gtest.h>


BLUE_DEFINE_LOG_CATEGORY( LogLoggerTest, Blue::LogLevel::Trace );
BLUE_DEFINE_LOG_CATEGORY( LogLoggerWarningOnlyTest, Blue::LogLevel::Warning );

#if BLUE_ENABLE_LOGGING
namespace
{
struct CountingSinkContext final
{
  Blue::AtomicUint64 EventCount{ 0 };
  Blue::AtomicUint64 ErrorCount{ 0 };
  Blue::AtomicUint64 InvalidEventCount{ 0 };
  Blue::AtomicUint64 FlushCount{ 0 };
  Blue::AtomicUint64 LastSequence{ 0 };
};

struct LoggingThreadContext final
{
  Blue::Uint32 Iterations = 0;
};

struct RecursiveSinkContext final
{
  Blue::AtomicUint64 Calls{ 0 };
};

void CountingSinkWrite( void* userData, const Blue::LogEvent& event )
{
  CountingSinkContext* context = static_cast< CountingSinkContext* >( userData );
  context->EventCount.FetchAdd( 1, Blue::MemoryOrder::Relaxed );

  if ( event.Level >= Blue::LogLevel::Error )
  {
    context->ErrorCount.FetchAdd( 1, Blue::MemoryOrder::Relaxed );
  }

  if ( !event.Message || !event.Category || !event.Category->Name || event.Thread == 0 || event.Sequence == 0 )
  {
    context->InvalidEventCount.FetchAdd( 1, Blue::MemoryOrder::Relaxed );
  }

  Blue::Uint64 previousSequence = context->LastSequence.Load( Blue::MemoryOrder::Relaxed );
  while (
    event.Sequence > previousSequence &&
    !context->LastSequence.CompareExchange( previousSequence, event.Sequence, Blue::MemoryOrder::AcquireRelease ) )
  {
  }
}

void CountingSinkFlush( void* userData )
{
  CountingSinkContext* context = static_cast< CountingSinkContext* >( userData );
  context->FlushCount.FetchAdd( 1, Blue::MemoryOrder::Relaxed );
}

void RecursiveSinkWrite( void* userData, const Blue::LogEvent& event )
{
  RecursiveSinkContext* context = static_cast< RecursiveSinkContext* >( userData );
  context->Calls.FetchAdd( 1, Blue::MemoryOrder::Relaxed );

  Blue::LogEvent recursiveEvent = event;
  recursiveEvent.Message = "recursive log event must be dropped";
  Blue::WriteLog( recursiveEvent );
}

Blue::Uint32 LoggingThreadEntry( void* userData )
{
  LoggingThreadContext* context = static_cast< LoggingThreadContext* >( userData );
  Blue::SetCurrentThreadName( "BlueLogWorker" );

  for ( Blue::Uint32 index = 0; index < context->Iterations; ++index )
  {
    BLUE_LOG_INFO( LogLoggerTest, "thread-safe logger test event" );
  }

  return context->Iterations;
}

Blue::LogSink MakeCountingSink( CountingSinkContext& context, Blue::LogLevel level = Blue::LogLevel::Trace )
{
  Blue::LogSink sink;
  sink.Context = &context;
  sink.Write = &CountingSinkWrite;
  sink.Flush = &CountingSinkFlush;
  sink.MinimumLevel = level;
  return sink;
}

Blue::LogSink MakeRecursiveSink( RecursiveSinkContext& context )
{
  Blue::LogSink sink;
  sink.Context = &context;
  sink.Write = &RecursiveSinkWrite;
  sink.Flush = nullptr;
  sink.MinimumLevel = Blue::LogLevel::Trace;
  return sink;
}
} // namespace

static void TestLoggerRegisterClearAndFlush( )
{
  CountingSinkContext context;

  ASSERT_TRUE( Blue::InitializeLogger( ) );
  ASSERT_TRUE( Blue::RegisterLogSink( MakeCountingSink( context ) ) );

  Blue::LoggerStateSnapshot snapshot = Blue::GetLoggerStateSnapshot( );
  ASSERT_TRUE( snapshot.Initialized );
  ASSERT_TRUE( snapshot.SinkCount == 1 );

  BLUE_LOG_INFO( LogLoggerTest, "single log event" );
  Blue::FlushLogger( );

  ASSERT_TRUE( context.EventCount.Load( ) == 1 );
  ASSERT_TRUE( context.FlushCount.Load( ) == 1 );
  ASSERT_TRUE( context.InvalidEventCount.Load( ) == 0 );

  Blue::ClearLogSinks( );
  snapshot = Blue::GetLoggerStateSnapshot( );
  ASSERT_TRUE( snapshot.SinkCount == 0 );

  Blue::ShutdownLogger( );
}

static void TestLoggerLevelFiltering( )
{
  CountingSinkContext context;

  ASSERT_TRUE( Blue::InitializeLogger( ) );
  ASSERT_TRUE( Blue::RegisterLogSink( MakeCountingSink( context ) ) );

  BLUE_LOG_INFO( LogLoggerWarningOnlyTest, "this info event must be filtered" );
  BLUE_LOG_ERROR( LogLoggerWarningOnlyTest, "this error event must pass" );

  ASSERT_TRUE( context.EventCount.Load( ) == 1 );
  ASSERT_TRUE( context.ErrorCount.Load( ) == 1 );

  Blue::ShutdownLogger( );
}

static void TestLoggerSinkMinimumLevelFiltering( )
{
  CountingSinkContext context;

  ASSERT_TRUE( Blue::InitializeLogger( ) );
  ASSERT_TRUE( Blue::RegisterLogSink( MakeCountingSink( context, Blue::LogLevel::Error ) ) );

  BLUE_LOG_WARNING( LogLoggerTest, "this warning event must be filtered by sink" );
  BLUE_LOG_ERROR( LogLoggerTest, "this error event must pass sink filtering" );

  ASSERT_TRUE( context.EventCount.Load( ) == 1 );
  ASSERT_TRUE( context.ErrorCount.Load( ) == 1 );

  Blue::ShutdownLogger( );
}

static void TestLoggerThreadSafety( )
{
  constexpr Blue::Uint32 ThreadCount = 8;
  constexpr Blue::Uint32 Iterations = 3000;
  constexpr Blue::Uint64 ExpectedEvents = static_cast< Blue::Uint64 >( ThreadCount ) * Iterations;

  CountingSinkContext context;
  Blue::Thread threads[ ThreadCount ];
  LoggingThreadContext threadContexts[ ThreadCount ];

  ASSERT_TRUE( Blue::InitializeLogger( ) );
  ASSERT_TRUE( Blue::RegisterLogSink( MakeCountingSink( context ) ) );

  for ( Blue::Uint32 index = 0; index < ThreadCount; ++index )
  {
    threadContexts[ index ].Iterations = Iterations;

    Blue::ThreadCreateDesc desc;
    desc.Name = "BlueLogWorker";
    desc.Entry = &LoggingThreadEntry;
    desc.UserData = &threadContexts[ index ];

    ASSERT_TRUE( Blue::CreateThread( threads[ index ], desc ) );
  }

  for ( Blue::Uint32 index = 0; index < ThreadCount; ++index )
  {
    Blue::Uint32 exitCode = 0;
    ASSERT_TRUE( Blue::JoinThread( threads[ index ], &exitCode ) );
    ASSERT_TRUE( exitCode == Iterations );
  }

  Blue::LoggerStateSnapshot snapshot = Blue::GetLoggerStateSnapshot( );

  ASSERT_TRUE( context.EventCount.Load( ) == ExpectedEvents );
  ASSERT_TRUE( context.InvalidEventCount.Load( ) == 0 );
  ASSERT_TRUE( snapshot.WrittenEventCount == ExpectedEvents );
  ASSERT_TRUE( snapshot.DroppedEventCount == 0 );
  ASSERT_TRUE( context.LastSequence.Load( ) == ExpectedEvents );

  Blue::ShutdownLogger( );
}

static void TestLoggerRecursionGuard( )
{
  RecursiveSinkContext context;

  ASSERT_TRUE( Blue::InitializeLogger( ) );
  ASSERT_TRUE( Blue::RegisterLogSink( MakeRecursiveSink( context ) ) );

  BLUE_LOG_INFO( LogLoggerTest, "outer event" );

  Blue::LoggerStateSnapshot snapshot = Blue::GetLoggerStateSnapshot( );

  ASSERT_TRUE( context.Calls.Load( ) == 1 );
  ASSERT_TRUE( snapshot.WrittenEventCount == 1 );
  ASSERT_TRUE( snapshot.DroppedEventCount == 1 );

  Blue::ShutdownLogger( );
}
#endif
TEST( BlueSystemLoggerThreadSafetyTests, RunsSuccessfully )
{
#if BLUE_ENABLE_LOGGING
  TestLoggerRegisterClearAndFlush( );
  TestLoggerLevelFiltering( );
  TestLoggerSinkMinimumLevelFiltering( );
  TestLoggerThreadSafety( );
  TestLoggerRecursionGuard( );
#else
  GTEST_SKIP( ) << "Logger thread-safety tests require BLUE_ENABLE_LOGGING.";
#endif
}

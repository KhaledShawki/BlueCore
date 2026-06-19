#include <Blue/System/Assert.h>
#include <Blue/System/Debug.h>
#include <Blue/System/Log.h>
#include <Blue/System/SourceLocation.h>
#include <Blue/System/Types.h>

#include <gtest/gtest.h>

namespace
{
struct TestContext final
{
  Blue::Uint32 EventCount = 0;
  Blue::Bool SawAssertCategory = false;
  Blue::Bool SawExpectedLocation = false;
};

Blue::Bool g_AssertHandlerCalled = false;
Blue::AssertContext g_LastAssertContext{ };

void TestAssertHandler( const Blue::AssertContext& context )
{
  g_AssertHandlerCalled = true;
  g_LastAssertContext = context;
}

void TestLogSinkWrite( void* context, const Blue::LogEvent& event )
{
  TestContext* testContext = static_cast< TestContext* >( context );
  ++testContext->EventCount;

  if ( event.Category && event.Category->Name && event.Category->Name[ 0 ] == 'A' )
  {
    testContext->SawAssertCategory = true;
  }

  if ( event.File && event.Line != 0 )
  {
    testContext->SawExpectedLocation = true;
  }
}

class AssertHandlerScope final
{
  public:
  explicit AssertHandlerScope( Blue::AssertHandler handler ) { Blue::SetAssertHandler( handler ); }

  ~AssertHandlerScope( ) { Blue::SetAssertHandler( nullptr ); }

  AssertHandlerScope( const AssertHandlerScope& ) = delete;
  AssertHandlerScope& operator=( const AssertHandlerScope& ) = delete;
};

class LoggerScope final
{
  public:
  explicit LoggerScope( bool initialized )
      : Initialized( initialized )
  {}

  ~LoggerScope( )
  {
    if ( Initialized )
    {
      Blue::ShutdownLogger( );
    }
  }

  LoggerScope( const LoggerScope& ) = delete;
  LoggerScope& operator=( const LoggerScope& ) = delete;

  private:
  bool Initialized = false;
};
} // namespace

TEST( BlueSystemDiagnosticsTests, SourceLocationMacroCapturesValidCallSite )
{
  const Blue::SourceLocation location = BLUE_SOURCE_LOCATION( );

  ASSERT_NE( location.File, nullptr );
  ASSERT_NE( location.Function, nullptr );
  EXPECT_GT( location.Line, 0u );
}

TEST( BlueSystemDiagnosticsTests, CustomAssertHandlerReceivesReportedAssertion )
{
  const Blue::SourceLocation location = BLUE_SOURCE_LOCATION( );
  g_AssertHandlerCalled = false;
  g_LastAssertContext = Blue::AssertContext{ };

  const AssertHandlerScope handlerScope( &TestAssertHandler );

  const Blue::AssertContext handledContext{ "1 == 2", "handler path", location };
  Blue::ReportAssertion( handledContext );

  ASSERT_TRUE( g_AssertHandlerCalled );
  EXPECT_EQ( g_LastAssertContext.Expression, handledContext.Expression );
  EXPECT_EQ( g_LastAssertContext.Message, handledContext.Message );
}

TEST( BlueSystemDiagnosticsTests, AssertionReportsThroughLoggerSink )
{
  const Blue::SourceLocation location = BLUE_SOURCE_LOCATION( );

  ( void ) Blue::IsDebuggerAttached( );
  Blue::WriteDebugOutput( "BlueSystem diagnostics smoke test output\n" );

  const bool loggerInitialized = Blue::InitializeLogger( );
  ASSERT_TRUE( loggerInitialized );
  const LoggerScope loggerScope( loggerInitialized );

  TestContext testContext{ };
  Blue::LogSink sink{ };
  sink.Context = &testContext;
  sink.Write = &TestLogSinkWrite;
  sink.MinimumLevel = Blue::LogLevel::Trace;

  ASSERT_TRUE( Blue::RegisterLogSink( sink ) );

  const Blue::AssertContext loggedContext{ "false", "logger path", location };
  Blue::ReportAssertion( loggedContext );

  const Blue::LoggerStateSnapshot snapshot = Blue::GetLoggerStateSnapshot( );

  EXPECT_EQ( testContext.EventCount, 1u );
  EXPECT_TRUE( testContext.SawAssertCategory );
  EXPECT_TRUE( testContext.SawExpectedLocation );
  EXPECT_NE( snapshot.WrittenEventCount, 0u );
}

#include <Blue/System/Assert.h>
#include <Blue/System/Debug.h>
#include <Blue/System/Log.h>
#include <Blue/System/SourceLocation.h>
#include <Blue/System/Types.h>

#include <stdio.h>

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

int Fail( const char* message )
{
  fprintf( stderr, "FAILED: %s\n", message );
  return 1;
}
} // namespace

int main( )
{
  const Blue::SourceLocation location = BLUE_SOURCE_LOCATION( );
  if ( !location.File || !location.Function || location.Line == 0 )
  {
    return Fail( "BLUE_SOURCE_LOCATION returned invalid data" );
  }

  ( void ) Blue::IsDebuggerAttached( );
  Blue::WriteDebugOutput( "BlueSystem diagnostics smoke test output\n" );

  Blue::SetAssertHandler( &TestAssertHandler );

  Blue::AssertContext handledContext{ "1 == 2", "handler path", location };
  Blue::ReportAssertion( handledContext );

  if ( !g_AssertHandlerCalled )
  {
    return Fail( "custom assert handler was not called" );
  }

  if ( g_LastAssertContext.Expression != handledContext.Expression ||
       g_LastAssertContext.Message != handledContext.Message )
  {
    return Fail( "custom assert handler received unexpected context" );
  }

  Blue::SetAssertHandler( nullptr );

  if ( !Blue::InitializeLogger( ) )
  {
    return Fail( "logger initialization failed" );
  }

  TestContext testContext{ };
  Blue::LogSink sink{ };
  sink.Context = &testContext;
  sink.Write = &TestLogSinkWrite;
  sink.MinimumLevel = Blue::LogLevel::Trace;

  if ( !Blue::RegisterLogSink( sink ) )
  {
    return Fail( "failed to register test log sink" );
  }

  Blue::AssertContext loggedContext{ "false", "logger path", location };
  Blue::ReportAssertion( loggedContext );

  Blue::LoggerStateSnapshot snapshot = Blue::GetLoggerStateSnapshot( );

  Blue::ShutdownLogger( );

  if ( testContext.EventCount != 1 )
  {
    return Fail( "assertion did not produce exactly one logger event" );
  }

  if ( !testContext.SawAssertCategory )
  {
    return Fail( "assertion logger event did not use Assert category" );
  }

  if ( !testContext.SawExpectedLocation )
  {
    return Fail( "assertion logger event did not preserve source location" );
  }

  if ( snapshot.WrittenEventCount == 0 )
  {
    return Fail( "logger written counter was not updated" );
  }

  printf( "BlueSystem diagnostics tests passed.\n" );
  return 0;
}

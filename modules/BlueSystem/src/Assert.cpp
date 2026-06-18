#include <Blue/System/Assert.h>
#include <Blue/System/Debug.h>
#include <Blue/System/Log.h>
#include <Blue/System/SpinLock.h>
#include <Blue/System/Thread.h>

#include <stdio.h>

namespace Blue
{
namespace
{
SpinLock g_AssertLock;
AssertHandler g_AssertHandler = nullptr;
LogCategory g_AssertLogCategory{ "Assert", LogLevel::Error, true };

const Char* SafeString( const Char* value ) noexcept
{
  return value ? value : "";
}

void WriteAssertToDebugOutput( const AssertContext& context ) noexcept
{
  char buffer[ 2048 ];

  const int written = snprintf( buffer,
                                sizeof( buffer ),
                                "Assertion failed:\nExpression: %s\nMessage: %s\nFile: %s\nFunction: %s\nLine: %u\n",
                                SafeString( context.Expression ),
                                SafeString( context.Message ),
                                SafeString( context.Location.File ),
                                SafeString( context.Location.Function ),
                                static_cast< unsigned >( context.Location.Line ) );

  if ( written <= 0 )
  {
    return;
  }

  buffer[ sizeof( buffer ) - 1 ] = '\0';
  WriteDebugOutput( buffer );
}

void WriteAssertToLogger( const AssertContext& context ) noexcept
{
  const LoggerStateSnapshot loggerState = GetLoggerStateSnapshot( );
  if ( !loggerState.Initialized || loggerState.SinkCount == 0 )
  {
    return;
  }

  LogEvent event{ };
  event.Level = LogLevel::Error;
  event.Category = &g_AssertLogCategory;
  event.Message = context.Message ? context.Message : context.Expression;
  event.File = context.Location.File;
  event.Function = context.Location.Function;
  event.Line = context.Location.Line;
  event.Thread = GetCurrentThreadId( );

  WriteLog( event );
}
} // namespace

void SetAssertHandler( AssertHandler handler )
{
  ScopedSpinLock lock( g_AssertLock );
  g_AssertHandler = handler;
}

AssertHandler GetAssertHandler( )
{
  ScopedSpinLock lock( g_AssertLock );
  return g_AssertHandler;
}

void ReportAssertion( const AssertContext& context )
{
  AssertHandler handler = nullptr;

  {
    ScopedSpinLock lock( g_AssertLock );
    handler = g_AssertHandler;
  }

  if ( handler )
  {
    handler( context );
    return;
  }

  WriteAssertToDebugOutput( context );
  WriteAssertToLogger( context );
}

void ReportAssertion( const Char* expression, const Char* message, const Char* file, int line )
{
  AssertContext context = {
    expression,
    message,
    { file, "", static_cast< Uint32 >( line ) }
  };
  ReportAssertion( context );
}
} // namespace Blue

#include <Blue/System/Log.h>
#include <Blue/System/SpinLock.h>
#include <Blue/System/Threading/Atomic.h>

#include <stdio.h>

namespace Blue
{
namespace
{
struct LoggerState final
{
	SpinLock Lock;
	LogSink Sinks[ MaxLogSinkCount ] = { };
	Uint32 SinkCount = 0;
	AtomicBool Initialized{ false };
	AtomicUint64 NextSequence{ 0 };
	AtomicUint64 WrittenEventCount{ 0 };
	AtomicUint64 DroppedEventCount{ 0 };
};

LoggerState g_LoggerState;
thread_local Bool g_IsInsideLogWrite = false;

Bool IsValidSink( const LogSink& sink ) noexcept
{
	return sink.Write != nullptr;
}

FILE* SelectConsoleFile( LogLevel level ) noexcept
{
	return level >= LogLevel::Warning ? stderr : stdout;
}
} // namespace

const Char* GetLogLevelName( LogLevel level ) noexcept
{
	switch ( level )
	{
		case LogLevel::Trace :   return "Trace";
		case LogLevel::Debug :   return "Debug";
		case LogLevel::Info :    return "Info";
		case LogLevel::Warning : return "Warning";
		case LogLevel::Error :   return "Error";
		case LogLevel::Fatal :   return "Fatal";
		default :                return "Unknown";
	}
}

Bool InitializeLogger( ) noexcept
{
	ScopedSpinLock guard( g_LoggerState.Lock );

	g_LoggerState.SinkCount = 0;
	g_LoggerState.NextSequence.Store( 0, MemoryOrder::Relaxed );
	g_LoggerState.WrittenEventCount.Store( 0, MemoryOrder::Relaxed );
	g_LoggerState.DroppedEventCount.Store( 0, MemoryOrder::Relaxed );
	g_LoggerState.Initialized.Store( true, MemoryOrder::Release );

	return true;
}

void ShutdownLogger( ) noexcept
{
	FlushLogger( );

	ScopedSpinLock guard( g_LoggerState.Lock );
	g_LoggerState.SinkCount = 0;
	g_LoggerState.Initialized.Store( false, MemoryOrder::Release );
}

Bool RegisterLogSink( const LogSink& sink ) noexcept
{
	if ( !IsValidSink( sink ) )
	{
		return false;
	}

	ScopedSpinLock guard( g_LoggerState.Lock );

	if ( g_LoggerState.SinkCount >= MaxLogSinkCount )
	{
		return false;
	}

	g_LoggerState.Sinks[ g_LoggerState.SinkCount ] = sink;
	++g_LoggerState.SinkCount;
	return true;
}

void ClearLogSinks( ) noexcept
{
	ScopedSpinLock guard( g_LoggerState.Lock );
	g_LoggerState.SinkCount = 0;
}

void FlushLogger( ) noexcept
{
	LogSink sinks[ MaxLogSinkCount ] = { };
	Uint32 sinkCount = 0;

	{
		ScopedSpinLock guard( g_LoggerState.Lock );
		sinkCount = g_LoggerState.SinkCount;

		for ( Uint32 index = 0; index < sinkCount; ++index )
		{
			sinks[ index ] = g_LoggerState.Sinks[ index ];
		}
	}

	for ( Uint32 index = 0; index < sinkCount; ++index )
	{
		if ( sinks[ index ].Flush )
		{
			sinks[ index ].Flush( sinks[ index ].Context );
		}
	}
}

Bool ShouldLog( const LogCategory& category, LogLevel level ) noexcept
{
	return category.Enabled && level >= category.MinimumLevel;
}

void WriteLog( LogEvent event ) noexcept
{
	if ( !g_LoggerState.Initialized.Load( MemoryOrder::Acquire ) )
	{
		g_LoggerState.DroppedEventCount.FetchAdd( 1, MemoryOrder::Relaxed );
		return;
	}

	if ( g_IsInsideLogWrite )
	{
		g_LoggerState.DroppedEventCount.FetchAdd( 1, MemoryOrder::Relaxed );
		return;
	}

	if ( event.Category && !ShouldLog( *event.Category, event.Level ) )
	{
		return;
	}

	event.Sequence = g_LoggerState.NextSequence.FetchAdd( 1, MemoryOrder::Relaxed ) + 1;

	LogSink sinks[ MaxLogSinkCount ] = { };
	Uint32 sinkCount = 0;

	{
		ScopedSpinLock guard( g_LoggerState.Lock );
		sinkCount = g_LoggerState.SinkCount;

		for ( Uint32 index = 0; index < sinkCount; ++index )
		{
			sinks[ index ] = g_LoggerState.Sinks[ index ];
		}
	}

	if ( sinkCount == 0 )
	{
		g_LoggerState.DroppedEventCount.FetchAdd( 1, MemoryOrder::Relaxed );
		return;
	}

	g_IsInsideLogWrite = true;

	Uint32 writtenSinkCount = 0;
	for ( Uint32 index = 0; index < sinkCount; ++index )
	{
		if ( !sinks[ index ].Write || event.Level < sinks[ index ].MinimumLevel )
		{
			continue;
		}

		sinks[ index ].Write( sinks[ index ].Context, event );
		++writtenSinkCount;
	}

	g_IsInsideLogWrite = false;

	if ( writtenSinkCount > 0 )
	{
		g_LoggerState.WrittenEventCount.FetchAdd( 1, MemoryOrder::Relaxed );
	}
	else
	{
		g_LoggerState.DroppedEventCount.FetchAdd( 1, MemoryOrder::Relaxed );
	}
}

LoggerStateSnapshot GetLoggerStateSnapshot( ) noexcept
{
	LoggerStateSnapshot snapshot{ };

	{
		ScopedSpinLock guard( g_LoggerState.Lock );
		snapshot.SinkCount = g_LoggerState.SinkCount;
	}

	snapshot.Initialized = g_LoggerState.Initialized.Load( MemoryOrder::Acquire );
	snapshot.WrittenEventCount = g_LoggerState.WrittenEventCount.Load( MemoryOrder::Relaxed );
	snapshot.DroppedEventCount = g_LoggerState.DroppedEventCount.Load( MemoryOrder::Relaxed );

	return snapshot;
}

namespace
{
void ConsoleLogSinkWrite( void*, const LogEvent& event )
{
	FILE* file = SelectConsoleFile( event.Level );
	const Char* levelName = GetLogLevelName( event.Level );
	const Char* categoryName = event.Category && event.Category->Name ? event.Category->Name : "General";
	const Char* message = event.Message ? event.Message : "";
	const Char* sourceFile = event.File ? event.File : "";
	const Char* function = event.Function ? event.Function : "";

	fprintf( file,
	         "[%llu][%s][%s][thread=%llu] %s (%s:%u %s)\n",
	         static_cast< unsigned long long >( event.Sequence ),
	         levelName,
	         categoryName,
	         static_cast< unsigned long long >( event.Thread ),
	         message,
	         sourceFile,
	         static_cast< unsigned >( event.Line ),
	         function );
}

void ConsoleLogSinkFlush( void* )
{
	fflush( stdout );
	fflush( stderr );
}
} // namespace

LogSink CreateConsoleLogSink( LogLevel minimumLevel ) noexcept
{
	LogSink sink{ };
	sink.Context = nullptr;
	sink.Write = &ConsoleLogSinkWrite;
	sink.Flush = &ConsoleLogSinkFlush;
	sink.MinimumLevel = minimumLevel;
	return sink;
}
} // namespace Blue

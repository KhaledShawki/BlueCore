#include <Blue/System/Debug.h>
#include <Blue/System/Types.h>

#include <signal.h>
#include <stdio.h>
#include <string.h>

namespace Blue
{
Bool IsDebuggerAttached( ) noexcept
{
	FILE* statusFile = fopen( "/proc/self/status", "r" );
	if ( !statusFile )
	{
		return false;
	}

	char line[ 256 ];
	Bool attached = false;

	while ( fgets( line, sizeof( line ), statusFile ) )
	{
		if ( strncmp( line, "TracerPid:", 10 ) == 0 )
		{
			int tracerPid = 0;
			if ( sscanf( line + 10, "%d", &tracerPid ) == 1 )
			{
				attached = tracerPid != 0;
			}
			break;
		}
	}

	fclose( statusFile );
	return attached;
}

void BreakIntoDebugger( ) noexcept
{
	raise( SIGTRAP );
}

void WriteDebugOutput( const Char* message ) noexcept
{
	if ( !message )
	{
		return;
	}

	fputs( message, stderr );
}
} // namespace Blue

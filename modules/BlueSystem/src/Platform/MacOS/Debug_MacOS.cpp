#include <Blue/System/Debug.h>
#include <Blue/System/Types.h>

#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/sysctl.h>
#include <sys/types.h>
#include <unistd.h>

namespace Blue
{
Bool IsDebuggerAttached( ) noexcept
{
	int mib[ 4 ] = { CTL_KERN, KERN_PROC, KERN_PROC_PID, getpid( ) };
	kinfo_proc info{ };
	size_t size = sizeof( info );

	if ( sysctl( mib, 4, &info, &size, nullptr, 0 ) != 0 )
	{
		return false;
	}

	return ( info.kp_proc.p_flag & P_TRACED ) != 0;
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

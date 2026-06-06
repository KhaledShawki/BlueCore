#include <Blue/System/Threading/Thread.h>

#include "../POSIX/POSIX_Thread.h"

#include <pthread.h>
#include <sched.h>
#include <string.h>

namespace Blue
{
namespace
{
constexpr Size MacOSThreadNameCapacity = 64;

Bool MapPriorityToSchedulingParameters( ThreadPriority priority, int& outPolicy, sched_param& outParameters ) noexcept
{
	memset( &outParameters, 0, sizeof( outParameters ) );

	switch ( priority )
	{
		case ThreadPriority::Low :
		case ThreadPriority::Normal :
			outPolicy = SCHED_OTHER;
			outParameters.sched_priority = 0;
			return true;

		case ThreadPriority::High :
		case ThreadPriority::Critical : {
			outPolicy = SCHED_RR;
			const int minPriority = sched_get_priority_min( outPolicy );
			const int maxPriority = sched_get_priority_max( outPolicy );
			if ( minPriority < 0 || maxPriority < 0 || maxPriority < minPriority )
			{
				return false;
			}

			const int range = maxPriority - minPriority;
			outParameters.sched_priority =
			    priority == ThreadPriority::Critical ? maxPriority : minPriority + ( range * 3 ) / 4;
			return true;
		}

		default : return false;
	}
}

Bool ApplyPriority( pthread_t thread, ThreadPriority priority ) noexcept
{
	int policy = SCHED_OTHER;
	sched_param parameters = { };
	if ( !MapPriorityToSchedulingParameters( priority, policy, parameters ) )
	{
		return false;
	}

	return pthread_setschedparam( thread, policy, &parameters ) == 0;
}
} // namespace

ThreadId GetCurrentThreadId( ) noexcept
{
	Uint64 threadId = 0;
	pthread_threadid_np( nullptr, &threadId );
	return static_cast< ThreadId >( threadId );
}

void SetCurrentThreadName( const Char* name ) noexcept
{
	if ( !name || name[ 0 ] == '\0' )
	{
		return;
	}

	Char truncatedName[ MacOSThreadNameCapacity ] = { };
	Internal::CopyThreadName( truncatedName, sizeof( truncatedName ), name );
	pthread_setname_np( truncatedName );
}

Bool SetThreadPriority( Thread& thread, ThreadPriority priority ) noexcept
{
	if ( !thread.Joinable )
	{
		return false;
	}

	return ApplyPriority( Internal::LoadNativeThreadHandle( thread.NativeHandle ), priority );
}

Bool SetCurrentThreadPriority( ThreadPriority priority ) noexcept
{
	return ApplyPriority( pthread_self( ), priority );
}

Bool SetThreadAffinity( Thread& thread, CpuAffinity affinity ) noexcept
{
	BLUE_UNUSED( thread );
	return !affinity.IsEnabled( );
}

Bool SetCurrentThreadAffinity( CpuAffinity affinity ) noexcept
{
	return !affinity.IsEnabled( );
}
} // namespace Blue

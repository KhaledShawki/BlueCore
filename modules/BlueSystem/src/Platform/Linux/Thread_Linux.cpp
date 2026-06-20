#include <Blue/System/Threading/Thread.h>

#include "../POSIX/POSIX_Thread.h"

#include <errno.h>
#include <pthread.h>
#include <sched.h>
#include <string.h>
#include <sys/syscall.h>
#include <unistd.h>

namespace Blue
{
namespace
{
constexpr Size LinuxThreadNameCapacity = 16;

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

void FillCpuSet( cpu_set_t& cpuSet, CpuAffinity affinity ) noexcept
{
  CPU_ZERO( &cpuSet );

  for ( Uint32 processorIndex = 0; processorIndex < 64; ++processorIndex )
  {
    if ( affinity.ContainsProcessor( processorIndex ) )
    {
      CPU_SET( processorIndex, &cpuSet );
    }
  }
}

Bool ApplyAffinity( pthread_t thread, CpuAffinity affinity ) noexcept
{
  if ( !affinity.IsEnabled( ) )
  {
    return true;
  }

  cpu_set_t cpuSet;
  FillCpuSet( cpuSet, affinity );
  return pthread_setaffinity_np( thread, sizeof( cpuSet ), &cpuSet ) == 0;
}
} // namespace

ThreadId GetCurrentThreadId( ) noexcept
{
  return static_cast< ThreadId >( syscall( SYS_gettid ) );
}

void SetCurrentThreadName( const Char* name ) noexcept
{
  if ( !name || name[ 0 ] == '\0' )
  {
    return;
  }

  Char truncatedName[ LinuxThreadNameCapacity ] = { };
  Internal::CopyThreadName( truncatedName, sizeof( truncatedName ), name );
  pthread_setname_np( pthread_self( ), truncatedName );
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
  if ( !thread.Joinable )
  {
    return false;
  }

  return ApplyAffinity( Internal::LoadNativeThreadHandle( thread.NativeHandle ), affinity );
}

Bool SetCurrentThreadAffinity( CpuAffinity affinity ) noexcept
{
  return ApplyAffinity( pthread_self( ), affinity );
}
} // namespace Blue

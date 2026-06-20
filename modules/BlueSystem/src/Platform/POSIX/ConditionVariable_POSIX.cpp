#include <Blue/System/Assert.h>
#include <Blue/System/ConditionVariable.h>

#include "POSIX_Synchronization.h"

#include <pthread.h>
#include <string.h>

namespace Blue
{
namespace
{
static_assert( sizeof( pthread_cond_t ) <= sizeof( NativeConditionVariableHandle ) );
static_assert( alignof( NativeConditionVariableHandle ) >= alignof( pthread_cond_t ) );
static_assert( sizeof( pthread_mutex_t ) <= sizeof( NativeMutexHandle ) );
static_assert( alignof( NativeMutexHandle ) >= alignof( pthread_mutex_t ) );

pthread_cond_t* GetNativeConditionVariable( NativeConditionVariableHandle& handle ) noexcept
{
  return reinterpret_cast< pthread_cond_t* >( handle.Storage );
}

pthread_mutex_t* GetNativeMutex( NativeMutexHandle& handle ) noexcept
{
  return reinterpret_cast< pthread_mutex_t* >( handle.Storage );
}

void ClearNativeConditionVariable( NativeConditionVariableHandle& handle ) noexcept
{
  memset( handle.Storage, 0, sizeof( handle.Storage ) );
}
} // namespace

Bool InitializeConditionVariable( ConditionVariable& conditionVariable ) noexcept
{
  if ( conditionVariable.Initialized )
  {
    return true;
  }

  ClearNativeConditionVariable( conditionVariable.NativeHandle );

  if ( !POSIX::InitializeConditionVariable( *GetNativeConditionVariable( conditionVariable.NativeHandle ) ) )
  {
    ClearNativeConditionVariable( conditionVariable.NativeHandle );
    return false;
  }

  conditionVariable.Initialized = true;
  return true;
}

void ShutdownConditionVariable( ConditionVariable& conditionVariable ) noexcept
{
  if ( !conditionVariable.Initialized )
  {
    return;
  }

  const int result = pthread_cond_destroy( GetNativeConditionVariable( conditionVariable.NativeHandle ) );
  BLUE_ASSERT( result == 0 );
  ( void ) result;

  ClearNativeConditionVariable( conditionVariable.NativeHandle );
  conditionVariable.Initialized = false;
}

void WaitConditionVariable( ConditionVariable& conditionVariable, Mutex& mutex ) noexcept
{
  BLUE_ASSERT( conditionVariable.Initialized );
  BLUE_ASSERT( mutex.Initialized );

  if ( !conditionVariable.Initialized || !mutex.Initialized )
  {
    return;
  }

  const int result = pthread_cond_wait( GetNativeConditionVariable( conditionVariable.NativeHandle ),
                                        GetNativeMutex( mutex.NativeHandle ) );

  BLUE_ASSERT( result == 0 );
  ( void ) result;
}

Bool WaitConditionVariableFor( ConditionVariable& conditionVariable, Mutex& mutex, TimeDuration timeout ) noexcept
{
  BLUE_ASSERT( conditionVariable.Initialized );
  BLUE_ASSERT( mutex.Initialized );

  if ( !conditionVariable.Initialized || !mutex.Initialized )
  {
    return false;
  }

  timespec deadline{ };
  if ( !POSIX::MakeAbsoluteTimeoutFromNow( timeout, deadline ) )
  {
    return false;
  }

  const int result = pthread_cond_timedwait( GetNativeConditionVariable( conditionVariable.NativeHandle ),
                                             GetNativeMutex( mutex.NativeHandle ),
                                             &deadline );

  if ( POSIX::IsTimeoutResult( result ) )
  {
    return false;
  }

  BLUE_ASSERT( result == 0 );
  return result == 0;
}

void NotifyOneConditionVariable( ConditionVariable& conditionVariable ) noexcept
{
  BLUE_ASSERT( conditionVariable.Initialized );
  if ( !conditionVariable.Initialized )
  {
    return;
  }

  const int result = pthread_cond_signal( GetNativeConditionVariable( conditionVariable.NativeHandle ) );
  BLUE_ASSERT( result == 0 );
  ( void ) result;
}

void NotifyAllConditionVariable( ConditionVariable& conditionVariable ) noexcept
{
  BLUE_ASSERT( conditionVariable.Initialized );
  if ( !conditionVariable.Initialized )
  {
    return;
  }

  const int result = pthread_cond_broadcast( GetNativeConditionVariable( conditionVariable.NativeHandle ) );
  BLUE_ASSERT( result == 0 );
  ( void ) result;
}
} // namespace Blue

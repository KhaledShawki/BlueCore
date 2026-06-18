#include <Blue/System/Assert.h>
#include <Blue/System/Event.h>

#include "POSIX_Synchronization.h"
#include <pthread.h>
#include <string.h>

namespace Blue
{
namespace
{
struct NativeEventState final
{
  pthread_mutex_t Mutex;
  pthread_cond_t Condition;
  EventResetMode ResetMode;
  Bool Signaled;
};

static_assert( sizeof( NativeEventState ) <= sizeof( NativeEventHandle ) );
static_assert( alignof( NativeEventHandle ) >= alignof( NativeEventState ) );

NativeEventState* GetNativeEvent( NativeEventHandle& handle ) noexcept
{
  return reinterpret_cast< NativeEventState* >( handle.Storage );
}

void ClearNativeEvent( NativeEventHandle& handle ) noexcept
{
  memset( handle.Storage, 0, sizeof( handle.Storage ) );
}

void ConsumeAutoResetSignalIfNeeded( NativeEventState& state ) noexcept
{
  if ( state.ResetMode == EventResetMode::Auto )
  {
    state.Signaled = false;
  }
}
} // namespace

Bool InitializeEvent( Event& event, const EventCreateDesc& desc ) noexcept
{
  if ( event.Initialized )
  {
    return true;
  }

  ClearNativeEvent( event.NativeHandle );
  NativeEventState* state = GetNativeEvent( event.NativeHandle );

  if ( pthread_mutex_init( &state->Mutex, nullptr ) != 0 )
  {
    ClearNativeEvent( event.NativeHandle );
    return false;
  }

  if ( !POSIX::InitializeConditionVariable( state->Condition ) )
  {
    pthread_mutex_destroy( &state->Mutex );
    ClearNativeEvent( event.NativeHandle );
    return false;
  }

  state->ResetMode = desc.ResetMode;
  state->Signaled = desc.InitiallySignaled;
  event.Initialized = true;
  return true;
}

void ShutdownEvent( Event& event ) noexcept
{
  if ( !event.Initialized )
  {
    return;
  }

  NativeEventState* state = GetNativeEvent( event.NativeHandle );

  const int condResult = pthread_cond_destroy( &state->Condition );
  const int mutexResult = pthread_mutex_destroy( &state->Mutex );
  BLUE_ASSERT( condResult == 0 );
  BLUE_ASSERT( mutexResult == 0 );
  ( void ) condResult;
  ( void ) mutexResult;

  ClearNativeEvent( event.NativeHandle );
  event.Initialized = false;
}

void SignalEvent( Event& event ) noexcept
{
  BLUE_ASSERT( event.Initialized );
  if ( !event.Initialized )
  {
    return;
  }

  NativeEventState* state = GetNativeEvent( event.NativeHandle );
  const int lockResult = pthread_mutex_lock( &state->Mutex );
  BLUE_ASSERT( lockResult == 0 );
  ( void ) lockResult;

  state->Signaled = true;

  if ( state->ResetMode == EventResetMode::Manual )
  {
    const int broadcastResult = pthread_cond_broadcast( &state->Condition );
    BLUE_ASSERT( broadcastResult == 0 );
    ( void ) broadcastResult;
  }
  else
  {
    const int signalResult = pthread_cond_signal( &state->Condition );
    BLUE_ASSERT( signalResult == 0 );
    ( void ) signalResult;
  }

  const int unlockResult = pthread_mutex_unlock( &state->Mutex );
  BLUE_ASSERT( unlockResult == 0 );
  ( void ) unlockResult;
}

void ResetEvent( Event& event ) noexcept
{
  BLUE_ASSERT( event.Initialized );
  if ( !event.Initialized )
  {
    return;
  }

  NativeEventState* state = GetNativeEvent( event.NativeHandle );
  const int lockResult = pthread_mutex_lock( &state->Mutex );
  BLUE_ASSERT( lockResult == 0 );
  ( void ) lockResult;

  state->Signaled = false;

  const int unlockResult = pthread_mutex_unlock( &state->Mutex );
  BLUE_ASSERT( unlockResult == 0 );
  ( void ) unlockResult;
}

void WaitEvent( Event& event ) noexcept
{
  BLUE_ASSERT( event.Initialized );
  if ( !event.Initialized )
  {
    return;
  }

  NativeEventState* state = GetNativeEvent( event.NativeHandle );
  const int lockResult = pthread_mutex_lock( &state->Mutex );
  BLUE_ASSERT( lockResult == 0 );
  ( void ) lockResult;

  while ( !state->Signaled )
  {
    const int waitResult = pthread_cond_wait( &state->Condition, &state->Mutex );
    BLUE_ASSERT( waitResult == 0 );
    ( void ) waitResult;
  }

  ConsumeAutoResetSignalIfNeeded( *state );

  const int unlockResult = pthread_mutex_unlock( &state->Mutex );
  BLUE_ASSERT( unlockResult == 0 );
  ( void ) unlockResult;
}

Bool TryWaitEvent( Event& event ) noexcept
{
  BLUE_ASSERT( event.Initialized );
  if ( !event.Initialized )
  {
    return false;
  }

  NativeEventState* state = GetNativeEvent( event.NativeHandle );
  const int lockResult = pthread_mutex_lock( &state->Mutex );
  BLUE_ASSERT( lockResult == 0 );
  ( void ) lockResult;

  const Bool acquired = state->Signaled;
  if ( acquired )
  {
    ConsumeAutoResetSignalIfNeeded( *state );
  }

  const int unlockResult = pthread_mutex_unlock( &state->Mutex );
  BLUE_ASSERT( unlockResult == 0 );
  ( void ) unlockResult;

  return acquired;
}

Bool WaitEventFor( Event& event, TimeDuration timeout ) noexcept
{
  BLUE_ASSERT( event.Initialized );
  if ( !event.Initialized )
  {
    return false;
  }

  if ( timeout.Nanoseconds == 0 )
  {
    return TryWaitEvent( event );
  }

  timespec deadline{ };
  if ( !POSIX::MakeAbsoluteTimeoutFromNow( timeout, deadline ) )
  {
    return false;
  }

  NativeEventState* state = GetNativeEvent( event.NativeHandle );
  const int lockResult = pthread_mutex_lock( &state->Mutex );
  BLUE_ASSERT( lockResult == 0 );
  ( void ) lockResult;

  while ( !state->Signaled )
  {
    const int waitResult = pthread_cond_timedwait( &state->Condition, &state->Mutex, &deadline );
    if ( POSIX::IsTimeoutResult( waitResult ) )
    {
      break;
    }

    BLUE_ASSERT( waitResult == 0 );
    if ( waitResult != 0 )
    {
      break;
    }
  }

  const Bool acquired = state->Signaled;
  if ( acquired )
  {
    ConsumeAutoResetSignalIfNeeded( *state );
  }

  const int unlockResult = pthread_mutex_unlock( &state->Mutex );
  BLUE_ASSERT( unlockResult == 0 );
  ( void ) unlockResult;

  return acquired;
}
} // namespace Blue

#include <Blue/System/Assert.h>
#include <Blue/System/Semaphore.h>

#include "POSIX_Synchronization.h"

#include <pthread.h>
#include <string.h>

namespace Blue
{
namespace
{
struct NativeSemaphoreState final
{
	pthread_mutex_t Mutex;
	pthread_cond_t Condition;
	Uint32 Count;
	Uint32 MaximumCount;
};

static_assert( sizeof( NativeSemaphoreState ) <= sizeof( NativeSemaphoreHandle ) );
static_assert( alignof( NativeSemaphoreHandle ) >= alignof( NativeSemaphoreState ) );

NativeSemaphoreState* GetNativeSemaphore( NativeSemaphoreHandle& handle ) noexcept
{
	return reinterpret_cast< NativeSemaphoreState* >( handle.Storage );
}

void ClearNativeSemaphore( NativeSemaphoreHandle& handle ) noexcept
{
	memset( handle.Storage, 0, sizeof( handle.Storage ) );
}
} // namespace

Bool InitializeSemaphore( Semaphore& semaphore, const SemaphoreCreateDesc& desc ) noexcept
{
	if ( semaphore.Initialized )
	{
		return true;
	}

	BLUE_ASSERT( desc.MaximumCount > 0 );
	BLUE_ASSERT( desc.InitialCount <= desc.MaximumCount );

	if ( desc.MaximumCount == 0 || desc.InitialCount > desc.MaximumCount )
	{
		return false;
	}

	ClearNativeSemaphore( semaphore.NativeHandle );
	NativeSemaphoreState* state = GetNativeSemaphore( semaphore.NativeHandle );

	if ( pthread_mutex_init( &state->Mutex, nullptr ) != 0 )
	{
		ClearNativeSemaphore( semaphore.NativeHandle );
		return false;
	}

	if ( !POSIX::InitializeConditionVariable( state->Condition ) )
	{
		pthread_mutex_destroy( &state->Mutex );
		ClearNativeSemaphore( semaphore.NativeHandle );
		return false;
	}

	state->Count = desc.InitialCount;
	state->MaximumCount = desc.MaximumCount;
	semaphore.Initialized = true;
	return true;
}

void ShutdownSemaphore( Semaphore& semaphore ) noexcept
{
	if ( !semaphore.Initialized )
	{
		return;
	}

	NativeSemaphoreState* state = GetNativeSemaphore( semaphore.NativeHandle );

	const int condResult = pthread_cond_destroy( &state->Condition );
	const int mutexResult = pthread_mutex_destroy( &state->Mutex );
	BLUE_ASSERT( condResult == 0 );
	BLUE_ASSERT( mutexResult == 0 );
	( void ) condResult;
	( void ) mutexResult;

	ClearNativeSemaphore( semaphore.NativeHandle );
	semaphore.Initialized = false;
}

void AcquireSemaphore( Semaphore& semaphore ) noexcept
{
	BLUE_ASSERT( semaphore.Initialized );
	if ( !semaphore.Initialized )
	{
		return;
	}

	NativeSemaphoreState* state = GetNativeSemaphore( semaphore.NativeHandle );
	const int lockResult = pthread_mutex_lock( &state->Mutex );
	BLUE_ASSERT( lockResult == 0 );
	( void ) lockResult;

	while ( state->Count == 0 )
	{
		const int waitResult = pthread_cond_wait( &state->Condition, &state->Mutex );
		BLUE_ASSERT( waitResult == 0 );
		( void ) waitResult;
	}

	--state->Count;

	const int unlockResult = pthread_mutex_unlock( &state->Mutex );
	BLUE_ASSERT( unlockResult == 0 );
	( void ) unlockResult;
}

Bool TryAcquireSemaphore( Semaphore& semaphore ) noexcept
{
	BLUE_ASSERT( semaphore.Initialized );
	if ( !semaphore.Initialized )
	{
		return false;
	}

	NativeSemaphoreState* state = GetNativeSemaphore( semaphore.NativeHandle );
	const int lockResult = pthread_mutex_lock( &state->Mutex );
	BLUE_ASSERT( lockResult == 0 );
	( void ) lockResult;

	const Bool acquired = state->Count > 0;
	if ( acquired )
	{
		--state->Count;
	}

	const int unlockResult = pthread_mutex_unlock( &state->Mutex );
	BLUE_ASSERT( unlockResult == 0 );
	( void ) unlockResult;

	return acquired;
}

Bool AcquireSemaphoreFor( Semaphore& semaphore, TimeDuration timeout ) noexcept
{
	BLUE_ASSERT( semaphore.Initialized );
	if ( !semaphore.Initialized )
	{
		return false;
	}

	if ( timeout.Nanoseconds == 0 )
	{
		return TryAcquireSemaphore( semaphore );
	}

	timespec deadline{ };
	if ( !POSIX::MakeAbsoluteTimeoutFromNow( timeout, deadline ) )
	{
		return false;
	}

	NativeSemaphoreState* state = GetNativeSemaphore( semaphore.NativeHandle );
	const int lockResult = pthread_mutex_lock( &state->Mutex );
	BLUE_ASSERT( lockResult == 0 );
	( void ) lockResult;

	Bool acquired = false;
	while ( state->Count == 0 )
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

	if ( state->Count > 0 )
	{
		--state->Count;
		acquired = true;
	}

	const int unlockResult = pthread_mutex_unlock( &state->Mutex );
	BLUE_ASSERT( unlockResult == 0 );
	( void ) unlockResult;

	return acquired;
}

Bool ReleaseSemaphore( Semaphore& semaphore, Uint32 count ) noexcept
{
	BLUE_ASSERT( semaphore.Initialized );
	BLUE_ASSERT( count > 0 );

	if ( !semaphore.Initialized || count == 0 )
	{
		return false;
	}

	NativeSemaphoreState* state = GetNativeSemaphore( semaphore.NativeHandle );
	const int lockResult = pthread_mutex_lock( &state->Mutex );
	BLUE_ASSERT( lockResult == 0 );
	( void ) lockResult;

	if ( count > state->MaximumCount - state->Count )
	{
		const int unlockResult = pthread_mutex_unlock( &state->Mutex );
		BLUE_ASSERT( unlockResult == 0 );
		( void ) unlockResult;
		return false;
	}

	state->Count += count;

	if ( count == 1 )
	{
		const int signalResult = pthread_cond_signal( &state->Condition );
		BLUE_ASSERT( signalResult == 0 );
		( void ) signalResult;
	}
	else
	{
		const int broadcastResult = pthread_cond_broadcast( &state->Condition );
		BLUE_ASSERT( broadcastResult == 0 );
		( void ) broadcastResult;
	}

	const int unlockResult = pthread_mutex_unlock( &state->Mutex );
	BLUE_ASSERT( unlockResult == 0 );
	( void ) unlockResult;
	return true;
}
} // namespace Blue

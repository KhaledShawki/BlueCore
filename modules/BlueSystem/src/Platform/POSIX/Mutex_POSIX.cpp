#include <Blue/System/Assert.h>
#include <Blue/System/Threading/Mutex.h>

#include <pthread.h>
#include <string.h>

namespace Blue
{
namespace
{
static_assert( sizeof( pthread_mutex_t ) <= sizeof( NativeMutexHandle ) );
static_assert( alignof( NativeMutexHandle ) >= alignof( pthread_mutex_t ) );

pthread_mutex_t* GetNativeMutex( NativeMutexHandle& handle ) noexcept
{
	return reinterpret_cast< pthread_mutex_t* >( handle.Storage );
}

void ClearNativeMutex( NativeMutexHandle& handle ) noexcept
{
	memset( handle.Storage, 0, sizeof( handle.Storage ) );
}
} // namespace

Bool InitializeMutex( Mutex& mutex ) noexcept
{
	if ( mutex.Initialized )
	{
		return true;
	}

	ClearNativeMutex( mutex.NativeHandle );

	pthread_mutexattr_t attributes;
	if ( pthread_mutexattr_init( &attributes ) != 0 )
	{
		return false;
	}

	pthread_mutexattr_settype( &attributes, PTHREAD_MUTEX_NORMAL );

	const int result = pthread_mutex_init( GetNativeMutex( mutex.NativeHandle ), &attributes );
	pthread_mutexattr_destroy( &attributes );

	if ( result != 0 )
	{
		ClearNativeMutex( mutex.NativeHandle );
		return false;
	}

	mutex.Initialized = true;
	return true;
}

void ShutdownMutex( Mutex& mutex ) noexcept
{
	if ( !mutex.Initialized )
	{
		return;
	}

	const int result = pthread_mutex_destroy( GetNativeMutex( mutex.NativeHandle ) );
	BLUE_ASSERT( result == 0 );
	( void ) result;

	ClearNativeMutex( mutex.NativeHandle );
	mutex.Initialized = false;
}

void AcquireMutex( Mutex& mutex ) noexcept
{
	BLUE_ASSERT( mutex.Initialized );
	if ( !mutex.Initialized )
	{
		return;
	}

	const int result = pthread_mutex_lock( GetNativeMutex( mutex.NativeHandle ) );
	BLUE_ASSERT( result == 0 );
	( void ) result;
}

Bool TryAcquireMutex( Mutex& mutex ) noexcept
{
	BLUE_ASSERT( mutex.Initialized );
	if ( !mutex.Initialized )
	{
		return false;
	}

	return pthread_mutex_trylock( GetNativeMutex( mutex.NativeHandle ) ) == 0;
}

void ReleaseMutex( Mutex& mutex ) noexcept
{
	BLUE_ASSERT( mutex.Initialized );
	if ( !mutex.Initialized )
	{
		return;
	}

	const int result = pthread_mutex_unlock( GetNativeMutex( mutex.NativeHandle ) );
	BLUE_ASSERT( result == 0 );
	( void ) result;
}
} // namespace Blue

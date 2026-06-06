#include <Blue/System/Assert.h>
#include <Blue/System/ConditionVariable.h>

#include <Blue/System/Platform/WindowsLean.h>
#include <string.h>

#include "Windows_Synchronization.h"

namespace Blue
{
namespace
{
static_assert( sizeof( CONDITION_VARIABLE ) <= sizeof( NativeConditionVariableHandle ) );
static_assert( alignof( NativeConditionVariableHandle ) >= alignof( CONDITION_VARIABLE ) );
static_assert( sizeof( SRWLOCK ) <= sizeof( NativeMutexHandle ) );
static_assert( alignof( NativeMutexHandle ) >= alignof( SRWLOCK ) );

CONDITION_VARIABLE* GetNativeConditionVariable( NativeConditionVariableHandle& handle ) noexcept
{
	return reinterpret_cast< CONDITION_VARIABLE* >( handle.Storage );
}

SRWLOCK* GetNativeMutex( NativeMutexHandle& handle ) noexcept
{
	return reinterpret_cast< SRWLOCK* >( handle.Storage );
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
	::InitializeConditionVariable( GetNativeConditionVariable( conditionVariable.NativeHandle ) );
	conditionVariable.Initialized = true;
	return true;
}

void ShutdownConditionVariable( ConditionVariable& conditionVariable ) noexcept
{
	if ( !conditionVariable.Initialized )
	{
		return;
	}

	    // CONDITION_VARIABLE does not require explicit destruction.
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

	const BOOL result = SleepConditionVariableSRW( GetNativeConditionVariable( conditionVariable.NativeHandle ),
	                                               GetNativeMutex( mutex.NativeHandle ),
	                                               INFINITE,
	                                               0 );

	BLUE_ASSERT( result != 0 );
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

	const BOOL result = SleepConditionVariableSRW( GetNativeConditionVariable( conditionVariable.NativeHandle ),
	                                               GetNativeMutex( mutex.NativeHandle ),
	                                               Windows::ToTimeoutMilliseconds( timeout ),
	                                               0 );

	if ( result == 0 && GetLastError( ) == ERROR_TIMEOUT )
	{
		return false;
	}

	BLUE_ASSERT( result != 0 );
	return result != 0;
}

void NotifyOneConditionVariable( ConditionVariable& conditionVariable ) noexcept
{
	BLUE_ASSERT( conditionVariable.Initialized );
	if ( !conditionVariable.Initialized )
	{
		return;
	}

	WakeConditionVariable( GetNativeConditionVariable( conditionVariable.NativeHandle ) );
}

void NotifyAllConditionVariable( ConditionVariable& conditionVariable ) noexcept
{
	BLUE_ASSERT( conditionVariable.Initialized );
	if ( !conditionVariable.Initialized )
	{
		return;
	}

	WakeAllConditionVariable( GetNativeConditionVariable( conditionVariable.NativeHandle ) );
}
} // namespace Blue

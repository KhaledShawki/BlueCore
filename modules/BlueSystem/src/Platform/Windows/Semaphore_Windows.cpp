#include <Blue/System/Assert.h>
#include <Blue/System/Semaphore.h>

#include <Blue/System/Platform/WindowsLean.h>
#include <limits.h>
#include <string.h>

#include "Windows_Synchronization.h"

namespace Blue
{
namespace
{
static_assert( sizeof( HANDLE ) <= sizeof( NativeSemaphoreHandle ) );

HANDLE& GetNativeSemaphore( NativeSemaphoreHandle& handle ) noexcept
{
	return *reinterpret_cast< HANDLE* >( handle.Storage );
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

	constexpr Uint32 MaxWin32SemaphoreCount = static_cast< Uint32 >( LONG_MAX );

	if ( desc.MaximumCount == 0 || desc.InitialCount > desc.MaximumCount || desc.MaximumCount > MaxWin32SemaphoreCount )
	{
		return false;
	}

	ClearNativeSemaphore( semaphore.NativeHandle );

	HANDLE handle = ::CreateSemaphoreW( nullptr,
	                                    static_cast< LONG >( desc.InitialCount ),
	                                    static_cast< LONG >( desc.MaximumCount ),
	                                    nullptr );

	if ( !handle )
	{
		return false;
	}

	GetNativeSemaphore( semaphore.NativeHandle ) = handle;
	semaphore.Initialized = true;
	return true;
}

void ShutdownSemaphore( Semaphore& semaphore ) noexcept
{
	if ( !semaphore.Initialized )
	{
		return;
	}

	HANDLE handle = GetNativeSemaphore( semaphore.NativeHandle );
	BLUE_ASSERT( handle != nullptr );

	if ( handle )
	{
		::CloseHandle( handle );
	}

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

	const DWORD result = ::WaitForSingleObject( GetNativeSemaphore( semaphore.NativeHandle ), INFINITE );
	BLUE_ASSERT( result == WAIT_OBJECT_0 );
	( void ) result;
}

Bool TryAcquireSemaphore( Semaphore& semaphore ) noexcept
{
	BLUE_ASSERT( semaphore.Initialized );
	if ( !semaphore.Initialized )
	{
		return false;
	}

	const DWORD result = ::WaitForSingleObject( GetNativeSemaphore( semaphore.NativeHandle ), 0 );
	return result == WAIT_OBJECT_0;
}

Bool AcquireSemaphoreFor( Semaphore& semaphore, TimeDuration timeout ) noexcept
{
	BLUE_ASSERT( semaphore.Initialized );
	if ( !semaphore.Initialized )
	{
		return false;
	}

	const DWORD result = ::WaitForSingleObject( GetNativeSemaphore( semaphore.NativeHandle ),
	                                            Windows::ToTimeoutMilliseconds( timeout ) );
	BLUE_ASSERT( result == WAIT_OBJECT_0 || result == WAIT_TIMEOUT );
	return result == WAIT_OBJECT_0;
}

Bool ReleaseSemaphore( Semaphore& semaphore, Uint32 count ) noexcept
{
	BLUE_ASSERT( semaphore.Initialized );
	BLUE_ASSERT( count > 0 );

	if ( !semaphore.Initialized || count == 0 )
	{
		return false;
	}

	return ::ReleaseSemaphore( GetNativeSemaphore( semaphore.NativeHandle ), static_cast< LONG >( count ), nullptr ) !=
	       0;
}
} // namespace Blue

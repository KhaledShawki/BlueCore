#include <Blue/System/Assert.h>
#include <Blue/System/Platform/WindowsLean.h>
#include <Blue/System/Threading/Mutex.h>

#include <string.h>

namespace Blue
{
namespace
{
static_assert( sizeof( SRWLOCK ) <= sizeof( NativeMutexHandle ) );
static_assert( alignof( NativeMutexHandle ) >= alignof( SRWLOCK ) );

SRWLOCK* GetNativeMutex( NativeMutexHandle& handle ) noexcept
{
  return reinterpret_cast< SRWLOCK* >( handle.Storage );
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
  InitializeSRWLock( GetNativeMutex( mutex.NativeHandle ) );
  mutex.Initialized = true;
  return true;
}

void ShutdownMutex( Mutex& mutex ) noexcept
{
  if ( !mutex.Initialized )
  {
    return;
  }

      // SRWLOCK does not require explicit destruction.
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

  AcquireSRWLockExclusive( GetNativeMutex( mutex.NativeHandle ) );
}

Bool TryAcquireMutex( Mutex& mutex ) noexcept
{
  BLUE_ASSERT( mutex.Initialized );
  if ( !mutex.Initialized )
  {
    return false;
  }

  return TryAcquireSRWLockExclusive( GetNativeMutex( mutex.NativeHandle ) ) != 0;
}

void ReleaseMutex( Mutex& mutex ) noexcept
{
  BLUE_ASSERT( mutex.Initialized );
  if ( !mutex.Initialized )
  {
    return;
  }

  ReleaseSRWLockExclusive( GetNativeMutex( mutex.NativeHandle ) );
}
} // namespace Blue

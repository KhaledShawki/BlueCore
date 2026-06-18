#include <Blue/System/Assert.h>
#include <Blue/System/Event.h>
#include <Blue/System/Platform/WindowsLean.h>

#include "Windows_Synchronization.h"
#include <string.h>

namespace Blue
{
namespace
{
static_assert( sizeof( HANDLE ) <= sizeof( NativeEventHandle ) );

HANDLE& GetNativeEvent( NativeEventHandle& handle ) noexcept
{
  return *reinterpret_cast< HANDLE* >( handle.Storage );
}

void ClearNativeEvent( NativeEventHandle& handle ) noexcept
{
  memset( handle.Storage, 0, sizeof( handle.Storage ) );
}
} // namespace

Bool InitializeEvent( Event& event, const EventCreateDesc& desc ) noexcept
{
  if ( event.Initialized )
  {
    return true;
  }

  ClearNativeEvent( event.NativeHandle );

  HANDLE handle = ::CreateEventW( nullptr,
                                  desc.ResetMode == EventResetMode::Manual ? TRUE : FALSE,
                                  desc.InitiallySignaled ? TRUE : FALSE,
                                  nullptr );

  if ( !handle )
  {
    return false;
  }

  GetNativeEvent( event.NativeHandle ) = handle;
  event.Initialized = true;
  return true;
}

void ShutdownEvent( Event& event ) noexcept
{
  if ( !event.Initialized )
  {
    return;
  }

  HANDLE handle = GetNativeEvent( event.NativeHandle );
  BLUE_ASSERT( handle != nullptr );

  if ( handle )
  {
    ::CloseHandle( handle );
  }

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

  const BOOL result = ::SetEvent( GetNativeEvent( event.NativeHandle ) );
  BLUE_ASSERT( result != 0 );
  ( void ) result;
}

void ResetEvent( Event& event ) noexcept
{
  BLUE_ASSERT( event.Initialized );
  if ( !event.Initialized )
  {
    return;
  }

  const BOOL result = ::ResetEvent( GetNativeEvent( event.NativeHandle ) );
  BLUE_ASSERT( result != 0 );
  ( void ) result;
}

void WaitEvent( Event& event ) noexcept
{
  BLUE_ASSERT( event.Initialized );
  if ( !event.Initialized )
  {
    return;
  }

  const DWORD result = ::WaitForSingleObject( GetNativeEvent( event.NativeHandle ), INFINITE );
  BLUE_ASSERT( result == WAIT_OBJECT_0 );
  ( void ) result;
}

Bool TryWaitEvent( Event& event ) noexcept
{
  BLUE_ASSERT( event.Initialized );
  if ( !event.Initialized )
  {
    return false;
  }

  const DWORD result = ::WaitForSingleObject( GetNativeEvent( event.NativeHandle ), 0 );
  return result == WAIT_OBJECT_0;
}

Bool WaitEventFor( Event& event, TimeDuration timeout ) noexcept
{
  BLUE_ASSERT( event.Initialized );
  if ( !event.Initialized )
  {
    return false;
  }

  const DWORD result =
    ::WaitForSingleObject( GetNativeEvent( event.NativeHandle ), Windows::ToTimeoutMilliseconds( timeout ) );
  BLUE_ASSERT( result == WAIT_OBJECT_0 || result == WAIT_TIMEOUT );
  return result == WAIT_OBJECT_0;
}
} // namespace Blue

#include "POSIX_Thread.h"
#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

namespace Blue
{
namespace
{
static_assert( sizeof( pthread_t ) <= sizeof( NativeThreadHandle ) );

Bool IsPriorityPolicyRequested( ThreadPriority priority ) noexcept
{
  return priority != ThreadPriority::Normal;
}

void ApplyCurrentThreadStartupPolicy( const Internal::POSIXThreadStartContext& context ) noexcept
{
  if ( context.HasName )
  {
    SetCurrentThreadName( context.Name );
  }

  if ( IsPriorityPolicyRequested( context.Priority ) )
  {
    BLUE_UNUSED( SetCurrentThreadPriority( context.Priority ) );
  }

  if ( context.Affinity.IsEnabled( ) )
  {
    BLUE_UNUSED( SetCurrentThreadAffinity( context.Affinity ) );
  }
}

void* ThreadEntryThunk( void* parameter ) noexcept
{
  Internal::POSIXThreadStartContext* context = static_cast< Internal::POSIXThreadStartContext* >( parameter );

  ThreadEntryFn entry = context->Entry;
  void* userData = context->UserData;

  ApplyCurrentThreadStartupPolicy( *context );
  free( context );

  if ( !entry )
  {
    return reinterpret_cast< void* >( static_cast< uintptr_t >( 1 ) );
  }

  const Uint32 result = entry( userData );
  return reinterpret_cast< void* >( static_cast< uintptr_t >( result ) );
}
} // namespace

namespace Internal
{
void ClearNativeThreadHandle( NativeThreadHandle& handle ) noexcept
{
  memset( handle.Storage, 0, sizeof( handle.Storage ) );
}

void StoreNativeThreadHandle( NativeThreadHandle& handle, pthread_t nativeHandle ) noexcept
{
  ClearNativeThreadHandle( handle );
  memcpy( handle.Storage, &nativeHandle, sizeof( nativeHandle ) );
}

pthread_t LoadNativeThreadHandle( const NativeThreadHandle& handle ) noexcept
{
  pthread_t nativeHandle = { };
  memcpy( &nativeHandle, handle.Storage, sizeof( nativeHandle ) );
  return nativeHandle;
}

void ResetThread( Thread& thread ) noexcept
{
  ClearNativeThreadHandle( thread.NativeHandle );
  thread.Id = 0;
  thread.Joinable = false;
}

ThreadId GetThreadIdFromPthread( pthread_t thread ) noexcept
{
  ThreadId id = 0;
  const Size copySize = sizeof( thread ) < sizeof( id ) ? sizeof( thread ) : sizeof( id );
  memcpy( &id, &thread, copySize );
  return id;
}

void CopyThreadName( Char* destination, Size destinationCapacity, const Char* source ) noexcept
{
  if ( !destination || destinationCapacity == 0 )
  {
    return;
  }

  destination[ 0 ] = '\0';

  if ( !source )
  {
    return;
  }

  Size index = 0;
  while ( index + 1 < destinationCapacity && source[ index ] != '\0' )
  {
    destination[ index ] = source[ index ];
    ++index;
  }

  destination[ index ] = '\0';
}
} // namespace Internal

Bool CreateThread( Thread& outThread, const ThreadCreateInfo& createInfo ) noexcept
{
  Internal::ResetThread( outThread );

  if ( !createInfo.Entry )
  {
    return false;
  }

  Internal::POSIXThreadStartContext* context =
    static_cast< Internal::POSIXThreadStartContext* >( malloc( sizeof( Internal::POSIXThreadStartContext ) ) );
  if ( !context )
  {
    return false;
  }

  *context = Internal::POSIXThreadStartContext{ };
  context->Entry = createInfo.Entry;
  context->UserData = createInfo.UserData;
  context->Priority = createInfo.Priority;
  context->Affinity = createInfo.Affinity;
  context->HasName = createInfo.Name && createInfo.Name[ 0 ] != '\0';
  Internal::CopyThreadName( context->Name, sizeof( context->Name ), createInfo.Name );

  pthread_attr_t attributes;
  const int attrInitResult = pthread_attr_init( &attributes );
  if ( attrInitResult != 0 )
  {
    free( context );
    return false;
  }

  if ( createInfo.StackSize > 0 )
  {
    const int stackSizeResult = pthread_attr_setstacksize( &attributes, createInfo.StackSize );
    if ( stackSizeResult != 0 )
    {
      pthread_attr_destroy( &attributes );
      free( context );
      return false;
    }
  }

  pthread_t nativeHandle = { };
  const int createResult = pthread_create( &nativeHandle, &attributes, &ThreadEntryThunk, context );
  pthread_attr_destroy( &attributes );

  if ( createResult != 0 )
  {
    free( context );
    return false;
  }

  Internal::StoreNativeThreadHandle( outThread.NativeHandle, nativeHandle );
  outThread.Id = Internal::GetThreadIdFromPthread( nativeHandle );
  outThread.Joinable = true;
  return true;
}

Bool JoinThread( Thread& thread, Uint32* outExitCode ) noexcept
{
  if ( !thread.Joinable )
  {
    return false;
  }

  pthread_t nativeHandle = Internal::LoadNativeThreadHandle( thread.NativeHandle );
  void* exitValue = nullptr;
  const int joinResult = pthread_join( nativeHandle, &exitValue );

  if ( joinResult != 0 )
  {
    return false;
  }

  if ( outExitCode )
  {
    *outExitCode = static_cast< Uint32 >( reinterpret_cast< uintptr_t >( exitValue ) );
  }

  Internal::ResetThread( thread );
  return true;
}

Bool DetachThread( Thread& thread ) noexcept
{
  if ( !thread.Joinable )
  {
    return false;
  }

  pthread_t nativeHandle = Internal::LoadNativeThreadHandle( thread.NativeHandle );
  const int detachResult = pthread_detach( nativeHandle );

  if ( detachResult != 0 )
  {
    return false;
  }

  Internal::ResetThread( thread );
  return true;
}

Bool IsThreadJoinable( const Thread& thread ) noexcept
{
  return thread.Joinable;
}
} // namespace Blue

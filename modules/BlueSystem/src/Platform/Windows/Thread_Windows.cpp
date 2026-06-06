#include <Blue/System/Threading/Thread.h>

#include <Blue/System/Platform/WindowsLean.h>
#include <limits.h>
#include <process.h>
#include <stdlib.h>
#include <string.h>

namespace Blue
{
namespace
{
struct ThreadStartContext final
{
	ThreadEntryFn Entry = nullptr;
	void* UserData = nullptr;
	Char Name[ 64 ] = { };
	Bool HasName = false;
};

static_assert( sizeof( HANDLE ) <= sizeof( NativeThreadHandle ) );

void ClearNativeHandle( NativeThreadHandle& handle )
{
	memset( handle.Storage, 0, sizeof( handle.Storage ) );
}

void StoreNativeHandle( NativeThreadHandle& handle, HANDLE nativeHandle )
{
	ClearNativeHandle( handle );
	memcpy( handle.Storage, &nativeHandle, sizeof( nativeHandle ) );
}

HANDLE LoadNativeHandle( const NativeThreadHandle& handle )
{
	HANDLE nativeHandle = nullptr;
	memcpy( &nativeHandle, handle.Storage, sizeof( nativeHandle ) );
	return nativeHandle;
}

void ResetThread( Thread& thread )
{
	ClearNativeHandle( thread.NativeHandle );
	thread.Id = 0;
	thread.Joinable = false;
}

int MapThreadPriority( ThreadPriority priority )
{
	switch ( priority )
	{
		case ThreadPriority::Low : return THREAD_PRIORITY_BELOW_NORMAL;

		case ThreadPriority::High : return THREAD_PRIORITY_ABOVE_NORMAL;

		case ThreadPriority::Critical : return THREAD_PRIORITY_HIGHEST;

		case ThreadPriority::Normal :
		default :                     return THREAD_PRIORITY_NORMAL;
	}
}

void CopyThreadName( Char* destination, Size destinationCapacity, const Char* source )
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

void ConvertUtf8ToWideBestEffort( const Char* source, WChar* destination, int destinationCapacity )
{
	if ( !destination || destinationCapacity <= 0 )
	{
		return;
	}

	destination[ 0 ] = L'\0';

	if ( !source || source[ 0 ] == '\0' )
	{
		return;
	}

	const int converted =
	    MultiByteToWideChar( CP_UTF8, MB_ERR_INVALID_CHARS, source, -1, destination, destinationCapacity );

	if ( converted > 0 )
	{
		destination[ destinationCapacity - 1 ] = L'\0';
		return;
	}

	int index = 0;
	while ( index + 1 < destinationCapacity && source[ index ] != '\0' )
	{
		const unsigned char value = static_cast< unsigned char >( source[ index ] );
		destination[ index ] = value < 128 ? static_cast< WChar >( value ) : L'?';
		++index;
	}

	destination[ index ] = L'\0';
}

unsigned __stdcall ThreadEntryThunk( void* parameter )
{
	ThreadStartContext* context = static_cast< ThreadStartContext* >( parameter );

	ThreadEntryFn entry = context->Entry;
	void* userData = context->UserData;

	if ( context->HasName )
	{
		SetCurrentThreadName( context->Name );
	}

	free( context );

	if ( !entry )
	{
		return 1;
	}

	return static_cast< unsigned >( entry( userData ) );
}
} // namespace

Bool CreateThread( Thread& outThread, const ThreadCreateInfo& desc ) noexcept
{
	ResetThread( outThread );

	if ( !desc.Entry || desc.StackSize > UINT_MAX )
	{
		return false;
	}

	ThreadStartContext* context = static_cast< ThreadStartContext* >( malloc( sizeof( ThreadStartContext ) ) );
	if ( !context )
	{
		return false;
	}

	*context = ThreadStartContext{ };
	context->Entry = desc.Entry;
	context->UserData = desc.UserData;
	context->HasName = desc.Name && desc.Name[ 0 ] != '\0';
	CopyThreadName( context->Name, sizeof( context->Name ), desc.Name );

	unsigned nativeThreadId = 0;
	const uintptr_t nativeHandleValue = _beginthreadex( nullptr,
	                                                    static_cast< unsigned >( desc.StackSize ),
	                                                    &ThreadEntryThunk,
	                                                    context,
	                                                    0,
	                                                    &nativeThreadId );

	if ( nativeHandleValue == 0 )
	{
		free( context );
		return false;
	}

	HANDLE nativeHandle = reinterpret_cast< HANDLE >( nativeHandleValue );
	StoreNativeHandle( outThread.NativeHandle, nativeHandle );
	outThread.Id = static_cast< ThreadId >( nativeThreadId );
	outThread.Joinable = true;

	BLUE_UNUSED( SetThreadPriority( outThread, desc.Priority ) );
	if ( desc.Affinity.IsEnabled( ) )
	{
		BLUE_UNUSED( SetThreadAffinity( outThread, desc.Affinity ) );
	}

	return true;
}

Bool JoinThread( Thread& thread, Uint32* outExitCode ) noexcept
{
	if ( !thread.Joinable )
	{
		return false;
	}

	HANDLE nativeHandle = LoadNativeHandle( thread.NativeHandle );
	if ( !nativeHandle )
	{
		ResetThread( thread );
		return false;
	}

	const DWORD waitResult = WaitForSingleObject( nativeHandle, INFINITE );
	if ( waitResult != WAIT_OBJECT_0 )
	{
		return false;
	}

	DWORD exitCode = 0;
	GetExitCodeThread( nativeHandle, &exitCode );

	if ( outExitCode )
	{
		*outExitCode = static_cast< Uint32 >( exitCode );
	}

	CloseHandle( nativeHandle );
	ResetThread( thread );
	return true;
}

Bool DetachThread( Thread& thread ) noexcept
{
	if ( !thread.Joinable )
	{
		return false;
	}

	HANDLE nativeHandle = LoadNativeHandle( thread.NativeHandle );
	if ( !nativeHandle )
	{
		ResetThread( thread );
		return false;
	}

	CloseHandle( nativeHandle );
	ResetThread( thread );
	return true;
}

Bool IsThreadJoinable( const Thread& thread ) noexcept
{
	return thread.Joinable;
}

ThreadId GetCurrentThreadId( ) noexcept
{
	return static_cast< ThreadId >( ::GetCurrentThreadId( ) );
}

void SetCurrentThreadName( const Char* name ) noexcept
{
	if ( !name || name[ 0 ] == '\0' )
	{
		return;
	}

	WChar wideName[ 64 ] = { };
	ConvertUtf8ToWideBestEffort( name, wideName, static_cast< int >( sizeof( wideName ) / sizeof( wideName[ 0 ] ) ) );

	if ( wideName[ 0 ] == L'\0' )
	{
		return;
	}

	SetThreadDescription( GetCurrentThread( ), wideName );
}

Bool SetThreadPriority( Thread& thread, ThreadPriority priority ) noexcept
{
	if ( !thread.Joinable )
	{
		return false;
	}

	HANDLE nativeHandle = LoadNativeHandle( thread.NativeHandle );
	if ( !nativeHandle )
	{
		return false;
	}

	return ::SetThreadPriority( nativeHandle, MapThreadPriority( priority ) ) != 0;
}

Bool SetCurrentThreadPriority( ThreadPriority priority ) noexcept
{
	return ::SetThreadPriority( GetCurrentThread( ), MapThreadPriority( priority ) ) != 0;
}

Bool SetThreadAffinity( Thread& thread, CpuAffinity affinity ) noexcept
{
	if ( !affinity.IsEnabled( ) )
	{
		return true;
	}

	if ( !thread.Joinable )
	{
		return false;
	}

	HANDLE nativeHandle = LoadNativeHandle( thread.NativeHandle );
	if ( !nativeHandle )
	{
		return false;
	}

	const DWORD_PTR affinityMask = static_cast< DWORD_PTR >( affinity.ProcessorMask );
	if ( static_cast< Uint64 >( affinityMask ) != affinity.ProcessorMask )
	{
		return false;
	}

	return SetThreadAffinityMask( nativeHandle, affinityMask ) != 0;
}

Bool SetCurrentThreadAffinity( CpuAffinity affinity ) noexcept
{
	if ( !affinity.IsEnabled( ) )
	{
		return true;
	}

	const DWORD_PTR affinityMask = static_cast< DWORD_PTR >( affinity.ProcessorMask );
	if ( static_cast< Uint64 >( affinityMask ) != affinity.ProcessorMask )
	{
		return false;
	}

	return SetThreadAffinityMask( GetCurrentThread( ), affinityMask ) != 0;
}

} // namespace Blue

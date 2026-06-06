#include <Blue/System/Mutex.h>
#include <Blue/System/Thread.h>
#include <Blue/System/Types.h>

#include <stdio.h>
#include <stdlib.h>

#define BLUE_TEST_EXPECT( expression )                                                                                 \
	do                                                                                                                 \
	{                                                                                                                  \
		if ( !( expression ) )                                                                                         \
		{                                                                                                              \
			fprintf( stderr, "Test failed: %s at %s:%d\n", #expression, __FILE__, __LINE__ );                          \
			abort( );                                                                                                  \
		}                                                                                                              \
	}                                                                                                                  \
	while ( false )

namespace
{
struct MutexIncrementContext final
{
	Blue::Mutex* Mutex;
	Blue::Uint32* Counter;
	Blue::Uint32 Iterations;
};

Blue::Uint32 MutexIncrementThreadEntry( void* userData )
{
	MutexIncrementContext* context = static_cast< MutexIncrementContext* >( userData );
	Blue::SetCurrentThreadName( "BlueMutexWorker" );

	for ( Blue::Uint32 index = 0; index < context->Iterations; ++index )
	{
		Blue::ScopedMutexLock lock( *context->Mutex );
		++( *context->Counter );
	}

	return 0;
}
} // namespace

static void TestMutexLifecycle( )
{
	Blue::Mutex mutex;

	BLUE_TEST_EXPECT( !Blue::IsMutexInitialized( mutex ) );
	BLUE_TEST_EXPECT( Blue::InitializeMutex( mutex ) );
	BLUE_TEST_EXPECT( Blue::IsMutexInitialized( mutex ) );

	Blue::AcquireMutex( mutex );
	Blue::ReleaseMutex( mutex );

	mutex.Acquire( );
	mutex.Release( );

	BLUE_TEST_EXPECT( Blue::TryAcquireMutex( mutex ) );
	Blue::ReleaseMutex( mutex );

	BLUE_TEST_EXPECT( mutex.TryAcquire( ) );
	mutex.Release( );

	Blue::ShutdownMutex( mutex );
	BLUE_TEST_EXPECT( !Blue::IsMutexInitialized( mutex ) );
}

static void TestScopedMutexLock( )
{
	Blue::Mutex mutex;
	BLUE_TEST_EXPECT( Blue::InitializeMutex( mutex ) );

	Blue::Uint32 value = 0;

	{
		Blue::ScopedMutexLock lock( mutex );
		++value;
	}

	BLUE_TEST_EXPECT( value == 1 );
	BLUE_TEST_EXPECT( Blue::TryAcquireMutex( mutex ) );
	Blue::ReleaseMutex( mutex );

	Blue::ShutdownMutex( mutex );
}

static void TestOwnedMutexLifecycle( )
{
	Blue::OwnedMutex mutex;
	BLUE_TEST_EXPECT( mutex.IsValid( ) );

	mutex.Acquire( );
	mutex.Release( );

	BLUE_TEST_EXPECT( mutex.TryAcquire( ) );
	mutex.Release( );
}

static void TestMutexWithMultipleThreads( )
{
	constexpr Blue::Uint32 ThreadCount = 8;
	constexpr Blue::Uint32 Iterations = 20000;

	Blue::Mutex mutex;
	BLUE_TEST_EXPECT( Blue::InitializeMutex( mutex ) );

	Blue::Uint32 counter = 0;
	Blue::Thread threads[ ThreadCount ];
	MutexIncrementContext contexts[ ThreadCount ];

	for ( Blue::Uint32 index = 0; index < ThreadCount; ++index )
	{
		contexts[ index ].Mutex = &mutex;
		contexts[ index ].Counter = &counter;
		contexts[ index ].Iterations = Iterations;

		Blue::ThreadCreateDesc desc;
		desc.Name = "BlueMutexWorker";
		desc.Entry = &MutexIncrementThreadEntry;
		desc.UserData = &contexts[ index ];

		BLUE_TEST_EXPECT( Blue::CreateThread( threads[ index ], desc ) );
	}

	for ( Blue::Uint32 index = 0; index < ThreadCount; ++index )
	{
		BLUE_TEST_EXPECT( Blue::JoinThread( threads[ index ] ) );
	}

	BLUE_TEST_EXPECT( counter == ThreadCount * Iterations );
	Blue::ShutdownMutex( mutex );
}

int main( )
{
	TestMutexLifecycle( );
	TestScopedMutexLock( );
	TestOwnedMutexLifecycle( );
	TestMutexWithMultipleThreads( );

	printf( "BlueSystem mutex tests passed.\n" );
	return 0;
}

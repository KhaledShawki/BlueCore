#include <Blue/System/Atomic.h>
#include <Blue/System/Platform.h>
#include <Blue/System/SpinLock.h>
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
struct ThreadIncrementContext final
{
	Blue::AtomicUint32* Counter;
	Blue::Uint32 Iterations;
	Blue::Uint32 ExitCode;
};

struct SpinLockThreadContext final
{
	Blue::SpinLock* Lock;
	Blue::Uint32* Counter;
	Blue::Uint32 Iterations;
};

Blue::Uint32 IncrementThreadEntry( void* userData )
{
	ThreadIncrementContext* context = static_cast< ThreadIncrementContext* >( userData );
	Blue::SetCurrentThreadName( "BlueIncrementWorker" );

	for ( Blue::Uint32 index = 0; index < context->Iterations; ++index )
	{
		context->Counter->FetchAdd( 1, Blue::MemoryOrder::Relaxed );
	}

	return context->ExitCode;
}

Blue::Uint32 SpinLockThreadEntry( void* userData )
{
	SpinLockThreadContext* context = static_cast< SpinLockThreadContext* >( userData );
	Blue::SetCurrentThreadName( "BlueSpinWorker" );

	for ( Blue::Uint32 index = 0; index < context->Iterations; ++index )
	{
		Blue::ScopedSpinLock lock( *context->Lock );
		++( *context->Counter );
	}

	return 0;
}

Blue::Uint32 DetachThreadEntry( void* userData )
{
	Blue::AtomicUint32* flag = static_cast< Blue::AtomicUint32* >( userData );
	Blue::SetCurrentThreadName( "BlueDetachWorker" );
	flag->Store( 1, Blue::MemoryOrder::Release );
	return 0;
}

Blue::Uint32 OwnedThreadEntry( void* userData )
{
	Blue::AtomicUint32* flag = static_cast< Blue::AtomicUint32* >( userData );
	Blue::SetCurrentThreadName( "BlueOwnedWorker" );
	flag->Store( 1, Blue::MemoryOrder::Release );
	return 23;
}

Blue::Uint32 PolicyThreadEntry( void* userData )
{
	Blue::AtomicUint32* flag = static_cast< Blue::AtomicUint32* >( userData );
	Blue::SetCurrentThreadName( "BluePolicyWorker" );
	BLUE_UNUSED( Blue::SetCurrentThreadPriority( Blue::ThreadPriority::Normal ) );
	BLUE_UNUSED( Blue::SetCurrentThreadAffinity( Blue::CpuAffinity::Any( ) ) );
	flag->Store( 1, Blue::MemoryOrder::Release );
	return 0;
}
} // namespace

static void TestThreadCreateJoin( )
{
	Blue::AtomicUint32 counter( 0 );
	ThreadIncrementContext context;
	context.Counter = &counter;
	context.Iterations = 10000;
	context.ExitCode = 17;

	Blue::Thread thread;
	Blue::ThreadCreateDesc desc;
	desc.Name = "BlueJoinWorker";
	desc.Entry = &IncrementThreadEntry;
	desc.UserData = &context;
	desc.StackSize = 0;
	desc.Priority = Blue::ThreadPriority::Normal;

	BLUE_TEST_EXPECT( Blue::CreateThread( thread, desc ) );
	BLUE_TEST_EXPECT( Blue::IsThreadJoinable( thread ) );
	BLUE_TEST_EXPECT( thread.Id != 0 );

	Blue::Uint32 exitCode = 0;
	BLUE_TEST_EXPECT( Blue::JoinThread( thread, &exitCode ) );
	BLUE_TEST_EXPECT( exitCode == context.ExitCode );
	BLUE_TEST_EXPECT( !Blue::IsThreadJoinable( thread ) );
	BLUE_TEST_EXPECT( counter.Load( Blue::MemoryOrder::Acquire ) == context.Iterations );
}

static void TestThreadDetach( )
{
	Blue::AtomicUint32 flag( 0 );

	Blue::Thread thread;
	Blue::ThreadCreateDesc desc;
	desc.Name = "BlueDetachWorker";
	desc.Entry = &DetachThreadEntry;
	desc.UserData = &flag;

	BLUE_TEST_EXPECT( Blue::CreateThread( thread, desc ) );
	BLUE_TEST_EXPECT( Blue::IsThreadJoinable( thread ) );
	BLUE_TEST_EXPECT( Blue::DetachThread( thread ) );
	BLUE_TEST_EXPECT( !Blue::IsThreadJoinable( thread ) );

	for ( Blue::Uint32 attempt = 0; attempt < 1000 && flag.Load( Blue::MemoryOrder::Acquire ) == 0; ++attempt )
	{
		Blue::YieldThread( );
	}

	BLUE_TEST_EXPECT( flag.Load( Blue::MemoryOrder::Acquire ) == 1 );
}

static void TestMultipleThreadsWithAtomics( )
{
	constexpr Blue::Uint32 ThreadCount = 8;
	constexpr Blue::Uint32 Iterations = 25000;

	Blue::AtomicUint32 counter( 0 );
	Blue::Thread threads[ ThreadCount ];
	ThreadIncrementContext contexts[ ThreadCount ];

	for ( Blue::Uint32 index = 0; index < ThreadCount; ++index )
	{
		contexts[ index ].Counter = &counter;
		contexts[ index ].Iterations = Iterations;
		contexts[ index ].ExitCode = index;

		Blue::ThreadCreateDesc desc;
		desc.Name = "BlueAtomicWorker";
		desc.Entry = &IncrementThreadEntry;
		desc.UserData = &contexts[ index ];

		BLUE_TEST_EXPECT( Blue::CreateThread( threads[ index ], desc ) );
	}

	for ( Blue::Uint32 index = 0; index < ThreadCount; ++index )
	{
		Blue::Uint32 exitCode = 0;
		BLUE_TEST_EXPECT( Blue::JoinThread( threads[ index ], &exitCode ) );
		BLUE_TEST_EXPECT( exitCode == index );
	}

	BLUE_TEST_EXPECT( counter.Load( Blue::MemoryOrder::Acquire ) == ThreadCount * Iterations );
}

static void TestMultipleThreadsWithSpinLock( )
{
	constexpr Blue::Uint32 ThreadCount = 6;
	constexpr Blue::Uint32 Iterations = 10000;

	Blue::SpinLock lock;
	Blue::Uint32 counter = 0;
	Blue::Thread threads[ ThreadCount ];
	SpinLockThreadContext contexts[ ThreadCount ];

	for ( Blue::Uint32 index = 0; index < ThreadCount; ++index )
	{
		contexts[ index ].Lock = &lock;
		contexts[ index ].Counter = &counter;
		contexts[ index ].Iterations = Iterations;

		Blue::ThreadCreateDesc desc;
		desc.Name = "BlueSpinWorker";
		desc.Entry = &SpinLockThreadEntry;
		desc.UserData = &contexts[ index ];

		BLUE_TEST_EXPECT( Blue::CreateThread( threads[ index ], desc ) );
	}

	for ( Blue::Uint32 index = 0; index < ThreadCount; ++index )
	{
		BLUE_TEST_EXPECT( Blue::JoinThread( threads[ index ] ) );
	}

	BLUE_TEST_EXPECT( counter == ThreadCount * Iterations );
}

static void TestCurrentThreadUtilities( )
{
	Blue::ThreadId currentThreadId = Blue::GetCurrentThreadId( );
	BLUE_TEST_EXPECT( currentThreadId != 0 );

	Blue::SetCurrentThreadName( "BlueMainTest" );
	Blue::YieldThread( );
	Blue::SleepCurrentThread( 1 );
}

static void TestCpuAffinityHelpers( )
{
	constexpr Blue::CpuAffinity any = Blue::CpuAffinity::Any( );
	constexpr Blue::CpuAffinity first = Blue::CpuAffinity::FromProcessorIndex( 0 );
	constexpr Blue::CpuAffinity invalid = Blue::CpuAffinity::FromProcessorIndex( 64 );

	BLUE_TEST_EXPECT( !any.IsEnabled( ) );
	BLUE_TEST_EXPECT( first.IsEnabled( ) );
	BLUE_TEST_EXPECT( first.ContainsProcessor( 0 ) );
	BLUE_TEST_EXPECT( !first.ContainsProcessor( 1 ) );
	BLUE_TEST_EXPECT( !invalid.IsEnabled( ) );
}

static void TestOwnedThreadJoinLifecycle( )
{
	Blue::AtomicUint32 flag( 0 );

	Blue::ThreadCreateInfo createInfo;
	createInfo.Name = "BlueOwnedWorker";
	createInfo.Entry = &OwnedThreadEntry;
	createInfo.UserData = &flag;

	Blue::OwnedThread thread( createInfo );
	BLUE_TEST_EXPECT( thread.IsJoinable( ) );
	BLUE_TEST_EXPECT( thread.GetId( ) != 0 );

	Blue::Uint32 exitCode = 0;
	BLUE_TEST_EXPECT( thread.Join( &exitCode ) );
	BLUE_TEST_EXPECT( exitCode == 23 );
	BLUE_TEST_EXPECT( !thread.IsJoinable( ) );
	BLUE_TEST_EXPECT( flag.Load( Blue::MemoryOrder::Acquire ) == 1 );
}

static void TestOwnedThreadDetachLifecycle( )
{
	Blue::AtomicUint32 flag( 0 );

	Blue::OwnedThread thread;

	Blue::ThreadCreateInfo createInfo;
	createInfo.Name = "BlueOwnedDetach";
	createInfo.Entry = &DetachThreadEntry;
	createInfo.UserData = &flag;

	BLUE_TEST_EXPECT( thread.Create( createInfo ) );
	BLUE_TEST_EXPECT( thread.IsJoinable( ) );
	BLUE_TEST_EXPECT( thread.Detach( ) );
	BLUE_TEST_EXPECT( !thread.IsJoinable( ) );

	for ( Blue::Uint32 attempt = 0; attempt < 1000 && flag.Load( Blue::MemoryOrder::Acquire ) == 0; ++attempt )
	{
		Blue::YieldThread( );
	}

	BLUE_TEST_EXPECT( flag.Load( Blue::MemoryOrder::Acquire ) == 1 );
}

static void TestThreadPolicyApiSmoke( )
{
	Blue::AtomicUint32 flag( 0 );

	Blue::Thread thread;
	Blue::ThreadCreateInfo createInfo;
	createInfo.Name = "BluePolicyWorker";
	createInfo.Entry = &PolicyThreadEntry;
	createInfo.UserData = &flag;
	createInfo.Priority = Blue::ThreadPriority::Normal;
	createInfo.Affinity = Blue::CpuAffinity::Any( );

	BLUE_TEST_EXPECT( Blue::CreateThread( thread, createInfo ) );
	BLUE_TEST_EXPECT( Blue::SetThreadAffinity( thread, Blue::CpuAffinity::Any( ) ) );
	BLUE_UNUSED( Blue::SetThreadPriority( thread, Blue::ThreadPriority::Normal ) );

	const Blue::CpuAffinity firstProcessor = Blue::CpuAffinity::FromProcessorIndex( 0 );
	BLUE_UNUSED( Blue::SetThreadAffinity( thread, firstProcessor ) );

	BLUE_TEST_EXPECT( Blue::JoinThread( thread ) );
	BLUE_TEST_EXPECT( flag.Load( Blue::MemoryOrder::Acquire ) == 1 );

	BLUE_TEST_EXPECT( Blue::SetCurrentThreadAffinity( Blue::CpuAffinity::Any( ) ) );
	BLUE_UNUSED( Blue::SetCurrentThreadAffinity( firstProcessor ) );
	BLUE_UNUSED( Blue::SetCurrentThreadPriority( Blue::ThreadPriority::Normal ) );
}

int main( )
{
	TestCurrentThreadUtilities( );
	TestCpuAffinityHelpers( );
	TestThreadCreateJoin( );
	TestThreadDetach( );
	TestOwnedThreadJoinLifecycle( );
	TestOwnedThreadDetachLifecycle( );
	TestThreadPolicyApiSmoke( );
	TestMultipleThreadsWithAtomics( );
	TestMultipleThreadsWithSpinLock( );

	printf( "BlueSystem native threading tests passed.\n" );
	return 0;
}

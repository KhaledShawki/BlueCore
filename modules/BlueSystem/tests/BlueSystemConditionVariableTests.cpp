#include <Blue/System/Atomic.h>
#include <Blue/System/ConditionVariable.h>
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
struct ConditionVariableContext final
{
	Blue::ConditionVariable* ConditionVariable;
	Blue::Mutex* Mutex;
	Blue::AtomicUint32* WaitingCount;
	Blue::Uint32* Ready;
	Blue::Uint32* Completed;
};

Blue::Uint32 ConditionVariableWorkerEntry( void* userData )
{
	ConditionVariableContext* context = static_cast< ConditionVariableContext* >( userData );
	Blue::SetCurrentThreadName( "BlueConditionWorker" );

	context->Mutex->Acquire( );
	context->WaitingCount->FetchAdd( 1, Blue::MemoryOrder::Release );

	while ( *context->Ready == 0 )
	{
		context->ConditionVariable->Wait( *context->Mutex );
	}

	++( *context->Completed );
	context->Mutex->Release( );
	return 0;
}
} // namespace

static void TestConditionVariableLifecycle( )
{
	Blue::ConditionVariable conditionVariable;
	BLUE_TEST_EXPECT( !Blue::IsConditionVariableInitialized( conditionVariable ) );
	BLUE_TEST_EXPECT( Blue::InitializeConditionVariable( conditionVariable ) );
	BLUE_TEST_EXPECT( Blue::IsConditionVariableInitialized( conditionVariable ) );
	Blue::NotifyOneConditionVariable( conditionVariable );
	Blue::NotifyAllConditionVariable( conditionVariable );
	Blue::ShutdownConditionVariable( conditionVariable );
	BLUE_TEST_EXPECT( !Blue::IsConditionVariableInitialized( conditionVariable ) );
}

static void TestOwnedConditionVariableAndTimedWait( )
{
	Blue::OwnedMutex mutex;
	Blue::OwnedConditionVariable conditionVariable;

	BLUE_TEST_EXPECT( mutex.IsValid( ) );
	BLUE_TEST_EXPECT( conditionVariable.IsValid( ) );

	mutex.Acquire( );
	BLUE_TEST_EXPECT( !conditionVariable.WaitFor( mutex.Get( ), Blue::MakeTimeDurationFromMilliseconds( 5 ) ) );
	mutex.Release( );
}

static void TestConditionVariableNotifyAll( )
{
	constexpr Blue::Uint32 ThreadCount = 6;

	Blue::Mutex mutex;
	Blue::ConditionVariable conditionVariable;
	BLUE_TEST_EXPECT( Blue::InitializeMutex( mutex ) );
	BLUE_TEST_EXPECT( Blue::InitializeConditionVariable( conditionVariable ) );

	Blue::AtomicUint32 waitingCount( 0 );
	Blue::Uint32 ready = 0;
	Blue::Uint32 completed = 0;
	Blue::Thread threads[ ThreadCount ];
	ConditionVariableContext context;
	context.ConditionVariable = &conditionVariable;
	context.Mutex = &mutex;
	context.WaitingCount = &waitingCount;
	context.Ready = &ready;
	context.Completed = &completed;

	for ( Blue::Uint32 index = 0; index < ThreadCount; ++index )
	{
		Blue::ThreadCreateDesc desc;
		desc.Name = "BlueConditionWorker";
		desc.Entry = &ConditionVariableWorkerEntry;
		desc.UserData = &context;
		BLUE_TEST_EXPECT( Blue::CreateThread( threads[ index ], desc ) );
	}

	for ( Blue::Uint32 attempt = 0; attempt < 10000 && waitingCount.Load( Blue::MemoryOrder::Acquire ) != ThreadCount;
	      ++attempt )
	{
		Blue::YieldThread( );
	}

	BLUE_TEST_EXPECT( waitingCount.Load( Blue::MemoryOrder::Acquire ) == ThreadCount );

	mutex.Acquire( );
	ready = 1;
	conditionVariable.NotifyAll( );
	mutex.Release( );

	for ( Blue::Uint32 index = 0; index < ThreadCount; ++index )
	{
		BLUE_TEST_EXPECT( Blue::JoinThread( threads[ index ] ) );
	}

	BLUE_TEST_EXPECT( completed == ThreadCount );
	Blue::ShutdownConditionVariable( conditionVariable );
	Blue::ShutdownMutex( mutex );
}

int main( )
{
	TestConditionVariableLifecycle( );
	TestOwnedConditionVariableAndTimedWait( );
	TestConditionVariableNotifyAll( );

	printf( "BlueSystem condition variable tests passed.\n" );
	return 0;
}

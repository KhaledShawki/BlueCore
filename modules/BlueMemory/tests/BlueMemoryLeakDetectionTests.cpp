#include <Blue/Memory/BlueNew.h>
#include <Blue/Memory/Invoker/RuntimeAllocationInvoker.h>
#include <Blue/Memory/MemorySystem.h>
#include <Blue/Memory/Pool/MemoryPoolRegistry.h>

#include <Blue/System/Log.h>
#include <Blue/System/Types.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
struct LeakLogContext
{
	Blue::Uint32 ErrorEventCount = 0;
	Blue::Bool SawMemoryLeakError = false;
};

struct LeakTestObject
{
	BLUE_USE_MEMORY_POOL( Test )

	LeakTestObject( ) noexcept
	    : Value( 0 )
	{}

	~LeakTestObject( ) noexcept = default;

	Blue::Uint32 Value;
};

static void LeakLogSinkWrite( void* context, const Blue::LogEvent& event )
{
	LeakLogContext* leakContext = static_cast< LeakLogContext* >( context );

	if ( event.Level < Blue::LogLevel::Error )
	{
		return;
	}

	++leakContext->ErrorEventCount;

	const bool isMemoryCategory = event.Category != nullptr && event.Category->Name != nullptr &&
	                              strcmp( event.Category->Name, "LogMemory" ) == 0;

	const bool isLeakMessage = event.Message != nullptr && strstr( event.Message, "live allocation" ) != nullptr;

	if ( isMemoryCategory && isLeakMessage )
	{
		leakContext->SawMemoryLeakError = true;
	}
}

static void InitializeLeakTestLogger( LeakLogContext& context )
{
	BLUE_TEST_EXPECT( Blue::InitializeLogger( ) );

	Blue::LogSink sink = { };
	sink.Context = &context;
	sink.Write = &LeakLogSinkWrite;
	sink.MinimumLevel = Blue::LogLevel::Error;

	BLUE_TEST_EXPECT( Blue::RegisterLogSink( sink ) );
}

static void ShutdownLeakTestLogger( )
{
	Blue::ShutdownLogger( );
}

static Blue::MemorySystemDesc CreateLeakDetectionMemorySystemDesc( )
{
	Blue::MemorySystemDesc desc = { };
	desc.EnableLeakDetection = true;
	return desc;
}
} // namespace

static void BlueMemoryLeakDetection_DetectsTypedAllocationLeak( )
{
	LeakLogContext logContext = { };
	InitializeLeakTestLogger( logContext );

	Blue::MemorySystemDesc desc = CreateLeakDetectionMemorySystemDesc( );
	BLUE_TEST_EXPECT( Blue::InitializeMemorySystem( desc ).Succeeded( ) );

	Blue::MemoryPoolStats before = { };
	BLUE_TEST_EXPECT( Blue::CaptureMemoryPoolStats( Blue::MemoryPoolId::Test, before ) );

	LeakTestObject* object = Blue::BlueNew< LeakTestObject >( );
	BLUE_TEST_EXPECT( object != nullptr );

	Blue::MemoryPoolStats afterAllocate = { };
	BLUE_TEST_EXPECT( Blue::CaptureMemoryPoolStats( Blue::MemoryPoolId::Test, afterAllocate ) );

	BLUE_TEST_EXPECT( afterAllocate.CurrentBytes == before.CurrentBytes + sizeof( LeakTestObject ) );

	Blue::ShutdownMemorySystem( );

	BLUE_TEST_EXPECT( logContext.SawMemoryLeakError );

	ShutdownLeakTestLogger( );
}

static void BlueMemoryLeakDetection_DetectsRuntimeAllocationLeak( )
{
	LeakLogContext logContext = { };
	InitializeLeakTestLogger( logContext );

	constexpr Blue::Size byteSize = 128;
	constexpr Blue::Size alignment = 16;
	constexpr Blue::MemoryPoolId pool = Blue::MemoryPoolId::Test;
	constexpr Blue::AllocationTag tag = Blue::AllocationTag::Test;

	Blue::MemorySystemDesc desc = CreateLeakDetectionMemorySystemDesc( );
	BLUE_TEST_EXPECT( Blue::InitializeMemorySystem( desc ).Succeeded( ) );

	Blue::MemoryPoolStats before = { };
	BLUE_TEST_EXPECT( Blue::CaptureMemoryPoolStats( pool, before ) );

	Blue::AllocationRequest request = BLUE_POOL_ALLOCATION_REQUEST( byteSize, alignment, tag, pool );

	void* pointer = Blue::BlueTryAllocate( request );
	BLUE_TEST_EXPECT( pointer != nullptr );

	Blue::MemoryPoolStats afterAllocate = { };
	BLUE_TEST_EXPECT( Blue::CaptureMemoryPoolStats( pool, afterAllocate ) );

	BLUE_TEST_EXPECT( afterAllocate.CurrentBytes == before.CurrentBytes + byteSize );

	Blue::ShutdownMemorySystem( );

	BLUE_TEST_EXPECT( logContext.SawMemoryLeakError );

	ShutdownLeakTestLogger( );
}

int main( )
{
	BlueMemoryLeakDetection_DetectsTypedAllocationLeak( );

	BlueMemoryLeakDetection_DetectsRuntimeAllocationLeak( );

	printf( "BlueMemory leak detection tests passed.\n" );
	return 0;
}

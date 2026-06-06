#include <Blue/Memory/Invoker/RuntimeAllocationInvoker.h>
#include <Blue/Memory/MemorySystem.h>
#include <Blue/Memory/Oom/OomReporter.h>
#include <Blue/Memory/Pool/MemoryPoolRegistry.h>
#include <Blue/Memory/PoolAllocator.h>
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

static void VerifyHeapReallocatePreservesExistingBytes( )
{
	Blue::Allocator allocator = Blue::GetDefaultAllocator( );
	Blue::AllocationResult initial =
	    Blue::Allocate( allocator, BLUE_ALLOCATION_REQUEST( 64, 16, Blue::AllocationTag::Test ) );
	BLUE_TEST_EXPECT( initial.Pointer != nullptr );

	Blue::Byte* bytes = static_cast< Blue::Byte* >( initial.Pointer );
	for ( Blue::Size index = 0; index < 64; ++index )
	{
		bytes[ index ] = static_cast< Blue::Byte >( index + 1 );
	}

	Blue::AllocationResult grown = Blue::Reallocate( allocator,
	                                                 initial.Pointer,
	                                                 64,
	                                                 BLUE_ALLOCATION_REQUEST( 128, 16, Blue::AllocationTag::Test ) );
	BLUE_TEST_EXPECT( grown.Pointer != nullptr );

	Blue::Byte* grownBytes = static_cast< Blue::Byte* >( grown.Pointer );
	for ( Blue::Size index = 0; index < 64; ++index )
	{
		BLUE_TEST_EXPECT( grownBytes[ index ] == static_cast< Blue::Byte >( index + 1 ) );
	}

	Blue::Free( allocator, grown.Pointer, grown.ByteSize, 16 );
}

static void VerifyAllocatorFreeUsesRequestPool( )
{
	Blue::Allocator allocator = Blue::GetDefaultAllocator( );

	Blue::MemoryPoolStats before = { };
	BLUE_TEST_EXPECT( Blue::CaptureMemoryPoolStats( Blue::MemoryPoolId::Resources, before ) );

	Blue::AllocationResult allocation = Blue::Allocate(
	    allocator,
	    BLUE_POOL_ALLOCATION_REQUEST( 96, 16, Blue::AllocationTag::Test, Blue::MemoryPoolId::Resources ) );
	BLUE_TEST_EXPECT( allocation.Pointer != nullptr );

	Blue::MemoryPoolStats during = { };
	BLUE_TEST_EXPECT( Blue::CaptureMemoryPoolStats( Blue::MemoryPoolId::Resources, during ) );
	BLUE_TEST_EXPECT( during.CurrentBytes == before.CurrentBytes + 96 );

	Blue::Free( allocator,
	            Blue::AllocationFreeRequest{ allocation.Pointer,
	                                         allocation.ByteSize,
	                                         16,
	                                         Blue::MemoryPoolId::Resources,
	                                         Blue::AllocationTag::Test } );

	Blue::MemoryPoolStats after = { };
	BLUE_TEST_EXPECT( Blue::CaptureMemoryPoolStats( Blue::MemoryPoolId::Resources, after ) );
	BLUE_TEST_EXPECT( after.CurrentBytes == before.CurrentBytes );
}

static void VerifyPoolAllocatorAlignmentAndBoundsHardening( )
{
	Blue::PoolAllocator pool;
	Blue::PoolAllocatorDesc poolDesc = { };
	poolDesc.ObjectSize = 24;
	poolDesc.ObjectAlignment = 32;
	poolDesc.ObjectCount = 2;
	poolDesc.Tag = Blue::AllocationTag::Test;
	poolDesc.BackingAllocator = Blue::GetDefaultAllocator( );

	BLUE_TEST_EXPECT( pool.Initialize( poolDesc ) );
	BLUE_TEST_EXPECT( pool.GetObjectSize( ) == 24 );
	BLUE_TEST_EXPECT( pool.GetObjectStride( ) == 32 );

	Blue::AllocationResult first = pool.Allocate( BLUE_ALLOCATION_REQUEST( 24, 32, Blue::AllocationTag::Test ) );
	Blue::AllocationResult second = pool.Allocate( BLUE_ALLOCATION_REQUEST( 24, 32, Blue::AllocationTag::Test ) );
	BLUE_TEST_EXPECT( first.Pointer != nullptr );
	BLUE_TEST_EXPECT( second.Pointer != nullptr );
	BLUE_TEST_EXPECT( ( reinterpret_cast< Blue::NativeUInt >( first.Pointer ) % 32 ) == 0 );
	BLUE_TEST_EXPECT( ( reinterpret_cast< Blue::NativeUInt >( second.Pointer ) % 32 ) == 0 );

	Blue::AllocationResult exhausted = pool.Allocate( BLUE_ALLOCATION_REQUEST( 24, 32, Blue::AllocationTag::Test ) );
	BLUE_TEST_EXPECT( exhausted.Pointer == nullptr );
	BLUE_TEST_EXPECT( pool.GetUsedCount( ) == 2 );

	pool.Free( first.Pointer, first.ByteSize, 32 );
	pool.Free( second.Pointer, second.ByteSize, 32 );
	BLUE_TEST_EXPECT( pool.GetUsedCount( ) == 0 );

	pool.Shutdown( );
}

int main( )
{
	Blue::OomReport reports[ 8 ] = { };
	Blue::MemorySystemDesc desc = { };
	desc.OomReportBuffer = reports;
	desc.OomReportCapacity = 8;
	BLUE_TEST_EXPECT( Blue::InitializeMemorySystem( desc ).Succeeded( ) );

	VerifyHeapReallocatePreservesExistingBytes( );
	VerifyAllocatorFreeUsesRequestPool( );
	VerifyPoolAllocatorAlignmentAndBoundsHardening( );

	Blue::AllocationRequest request =
	    BLUE_POOL_ALLOCATION_REQUEST( 128, 16, Blue::AllocationTag::Test, Blue::MemoryPoolId::Resources );
	void* pointer = Blue::BlueTryAllocate( request );
	BLUE_TEST_EXPECT( pointer != nullptr );

	Blue::MemoryPoolStats during = { };
	BLUE_TEST_EXPECT( Blue::CaptureMemoryPoolStats( Blue::MemoryPoolId::Resources, during ) );
	BLUE_TEST_EXPECT( during.CurrentBytes >= 128 );
	BLUE_TEST_EXPECT( during.AllocationCount >= 1 );

	Blue::BlueFree(
	    Blue::AllocationFreeRequest{ pointer, 128, 16, Blue::MemoryPoolId::Resources, Blue::AllocationTag::Test } );

	Blue::AllocationRequest invalid =
	    BLUE_POOL_ALLOCATION_REQUEST( 64, 3, Blue::AllocationTag::Test, Blue::MemoryPoolId::Resources );
	void* invalidPointer = Blue::BlueTryAllocate( invalid );
	BLUE_TEST_EXPECT( invalidPointer == nullptr );

	Blue::OomReport captured[ 8 ] = { };
	const Blue::Size reportCount = Blue::CaptureOomReports( captured, 8 );
	BLUE_TEST_EXPECT( reportCount > 0 );
	BLUE_TEST_EXPECT( captured[ 0 ].Reason == Blue::AllocationFailureReason::InvalidAlignment );
	BLUE_TEST_EXPECT( captured[ 0 ].NativeThreadId != 0 );

	Blue::ShutdownMemorySystem( );
	printf( "BlueMemory runtime allocation tests passed.\n" );
	return 0;
}

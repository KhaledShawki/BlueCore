#include <Blue/Memory/Allocator/SmallBlockAllocator.h>
#include <Blue/Memory/BlueNew.h>
#include <Blue/Memory/Invoker/RuntimeAllocationInvoker.h>
#include <Blue/Memory/MemorySystem.h>
#include <Blue/Memory/Pool/MemoryPoolRegistry.h>
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
struct SmallRendererObject
{
	BLUE_USE_MEMORY_POOL( Renderer )

	SmallRendererObject( Blue::Uint32 value ) noexcept
	    : Value( value )
	{}

	~SmallRendererObject( ) noexcept
	{
		Value = 0;
	}

	Blue::Uint32 Value = 0;
};

static_assert( sizeof( SmallRendererObject ) <= Blue::BlueSmallBlockMaxSize );
} // namespace

int main( )
{
	Blue::MemorySystemDesc desc = { };
	BLUE_TEST_EXPECT( Blue::InitializeMemorySystem( desc ).Succeeded( ) );

	BLUE_TEST_EXPECT( Blue::IsSmallBlockAllocationSupported( 1, 1 ) );
	BLUE_TEST_EXPECT( Blue::IsSmallBlockAllocationSupported( 64, 64 ) );
	BLUE_TEST_EXPECT( Blue::IsSmallBlockAllocationSupported( 128, 16 ) );
	BLUE_TEST_EXPECT( !Blue::IsSmallBlockAllocationSupported( Blue::BlueSmallBlockMaxSize + 1, 16 ) );
	BLUE_TEST_EXPECT( !Blue::IsSmallBlockAllocationSupported( 64, 3 ) );
	BLUE_TEST_EXPECT( Blue::GetSmallBlockClassSize( 17, 8 ) == 32 );
	BLUE_TEST_EXPECT( Blue::GetSmallBlockClassSize( 32, 64 ) == 64 );

	Blue::SmallBlockAllocatorStats initialStats = Blue::GetSmallBlockAllocatorStats( );
	SmallRendererObject* objects[ 256 ] = { };

	Blue::MemoryPoolStats before = { };
	BLUE_TEST_EXPECT( Blue::CaptureMemoryPoolStats( Blue::MemoryPoolId::Renderer, before ) );

	for ( Blue::Size index = 0; index < 256; ++index )
	{
		objects[ index ] = Blue::BlueNew< SmallRendererObject >( static_cast< Blue::Uint32 >( index ) );
		BLUE_TEST_EXPECT( objects[ index ] != nullptr );
		BLUE_TEST_EXPECT( objects[ index ]->Value == index );
		BLUE_TEST_EXPECT(
		    ( reinterpret_cast< Blue::NativeUInt >( objects[ index ] ) % alignof( SmallRendererObject ) ) == 0 );
	}

	Blue::MemoryPoolStats during = { };
	BLUE_TEST_EXPECT( Blue::CaptureMemoryPoolStats( Blue::MemoryPoolId::Renderer, during ) );
	BLUE_TEST_EXPECT( during.CurrentBytes == before.CurrentBytes + sizeof( SmallRendererObject ) * 256 );
	BLUE_TEST_EXPECT( during.AllocationCount == before.AllocationCount + 256 );

	for ( Blue::Size index = 0; index < 256; ++index )
	{
		Blue::BlueDelete( objects[ index ] );
	}

	Blue::MemoryPoolStats after = { };
	BLUE_TEST_EXPECT( Blue::CaptureMemoryPoolStats( Blue::MemoryPoolId::Renderer, after ) );
	BLUE_TEST_EXPECT( after.CurrentBytes == before.CurrentBytes );
	BLUE_TEST_EXPECT( after.FreeCount == before.FreeCount + 256 );

	Blue::SmallBlockAllocatorStats finalStats = Blue::GetSmallBlockAllocatorStats( );
	BLUE_TEST_EXPECT( finalStats.AllocateCount >= initialStats.AllocateCount + 256 );
	BLUE_TEST_EXPECT( finalStats.FreeCount >= initialStats.FreeCount + 256 );
	BLUE_TEST_EXPECT( finalStats.RefillCount > initialStats.RefillCount );
	BLUE_TEST_EXPECT( finalStats.SlabCount > initialStats.SlabCount );

	Blue::AllocationRequest request =
	    BLUE_POOL_ALLOCATION_REQUEST( 32, 16, Blue::AllocationTag::Test, Blue::MemoryPoolId::Resources );
	void* runtimePointer = Blue::BlueTryAllocate( request );
	BLUE_TEST_EXPECT( runtimePointer != nullptr );
	BLUE_TEST_EXPECT( ( reinterpret_cast< Blue::NativeUInt >( runtimePointer ) % 16 ) == 0 );
	Blue::BlueFree( Blue::AllocationFreeRequest{ runtimePointer,
	                                             32,
	                                             16,
	                                             Blue::MemoryPoolId::Resources,
	                                             Blue::AllocationTag::Test } );

	Blue::ShutdownMemorySystem( );
	printf( "BlueMemory small block allocator tests passed.\n" );
	return 0;
}

#include <Blue/Memory/MemorySystem.h>
#include <Blue/System/Time.h>
#include <stdio.h>

int main( )
{
	Blue::MemorySystemDesc desc = { };
	desc.EnableMetrics = true;
	Blue::InitializeMemorySystem( desc );

	Blue::Allocator allocator = Blue::GetDefaultAllocator( );
	const Blue::Uint64 begin = Blue::GetTimeNowNs( );

	for ( Blue::Uint32 index = 0; index < 100000; ++index )
	{
		Blue::AllocationResult result =
		    Blue::Allocate( allocator, BLUE_ALLOCATION_REQUEST( 64, 16, Blue::AllocationTag::Test ) );
		Blue::Free( allocator, result.Pointer, result.ByteSize, 16 );
	}

	const Blue::Uint64 end = Blue::GetTimeNowNs( );
	printf( "Allocator benchmark: %llu ns\n", static_cast< unsigned long long >( end - begin ) );

	Blue::ShutdownMemorySystem( );
	return 0;
}

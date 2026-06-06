#include <Blue/Memory/LinearAllocator.h>

namespace Blue
{
static uintptr_t AlignForward( uintptr_t address, Size alignment )
{
	const uintptr_t mask = static_cast< uintptr_t >( alignment - 1 );
	return ( address + mask ) & ~mask;
}

static AllocationResult LinearAllocate( void* context, const AllocationRequest& request )
{
	LinearAllocator* allocator = static_cast< LinearAllocator* >( context );
	const uintptr_t current = reinterpret_cast< uintptr_t >( allocator->Current );
	const uintptr_t aligned = AlignForward( current, request.Alignment );
	const uintptr_t next = aligned + request.ByteSize;

	if ( next > reinterpret_cast< uintptr_t >( allocator->End ) )
	{
		return { nullptr, 0 };
	}

	allocator->Current = reinterpret_cast< Byte* >( next );
	return { reinterpret_cast< void* >( aligned ), request.ByteSize };
}

static AllocationResult LinearReallocate( void*, void*, Size, const AllocationRequest& )
{
	return { nullptr, 0 };
}

static void LinearFree( void*, const AllocationFreeRequest& ) {}

void InitializeLinearAllocator( LinearAllocator& allocator, void* memory, Size size, AllocationTag defaultTag )
{
	BLUE_ASSERT( memory );
	BLUE_ASSERT( size > 0 );
	allocator.Start = static_cast< Byte* >( memory );
	allocator.Current = allocator.Start;
	allocator.End = allocator.Start + size;
	allocator.DefaultTag = defaultTag;
}

void ResetLinearAllocator( LinearAllocator& allocator )
{
	allocator.Current = allocator.Start;
}

Allocator MakeLinearAllocator( LinearAllocator& allocator )
{
	return Allocator{ &allocator, LinearAllocate, LinearReallocate, LinearFree };
}
} // namespace Blue

#pragma once

#include <Blue/Memory/Allocator.h>

namespace Blue
{
template< typename TAllocator >
struct AllocatorInvoker
{
	static AllocationResult Allocate( void* context, const AllocationRequest& request )
	{
		TAllocator* allocator = static_cast< TAllocator* >( context );
		return allocator->Allocate( request );
	}

	static AllocationResult Reallocate( void* context, void* pointer, Size oldSize, const AllocationRequest& request )
	{
		TAllocator* allocator = static_cast< TAllocator* >( context );
		return allocator->Reallocate( pointer, oldSize, request );
	}

	static void Free( void* context, const AllocationFreeRequest& request )
	{
		TAllocator* allocator = static_cast< TAllocator* >( context );
		allocator->Free( request );
	}

	static Allocator Make( TAllocator& allocator )
	{
		return Allocator{
		    &allocator,
		    &AllocatorInvoker< TAllocator >::Allocate,
		    &AllocatorInvoker< TAllocator >::Reallocate,
		    &AllocatorInvoker< TAllocator >::Free,
		};
	}
};
} // namespace Blue

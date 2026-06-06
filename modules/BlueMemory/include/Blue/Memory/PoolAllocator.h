#pragma once

#include <Blue/Memory/Allocator.h>
#include <Blue/Memory/Api.h>
#include <Blue/Memory/MemoryBlock.h>

namespace Blue
{
struct PoolAllocatorDesc
{
	Size ObjectSize;
	Size ObjectAlignment;
	Size ObjectCount;
	AllocationTag Tag;
	Allocator BackingAllocator;
};

class BLUE_MEMORY_API PoolAllocator
{
public:
	PoolAllocator( );

	bool Initialize( const PoolAllocatorDesc& desc );
	void Shutdown( );

	AllocationResult Allocate( const AllocationRequest& request );
	AllocationResult Reallocate( void* pointer, Size oldSize, const AllocationRequest& request );
	void Free( void* pointer, Size size, Size alignment );
	void Free( const AllocationFreeRequest& request );

	Size GetObjectSize( ) const;
	Size GetCapacity( ) const;
	Size GetUsedCount( ) const;
	Size GetObjectStride( ) const;

private:
	MemoryBlock m_Block;
	void* m_FreeList;
	Size m_ObjectSize;
	Size m_ObjectStride;
	Size m_ObjectAlignment;
	Size m_Capacity;
	Size m_UsedCount;
	Allocator m_BackingAllocator;
};
} // namespace Blue

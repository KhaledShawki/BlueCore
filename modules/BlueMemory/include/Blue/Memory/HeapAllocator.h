#pragma once

#include <Blue/Memory/Allocation/AllocationFreeRequest.h>
#include <Blue/Memory/Allocator.h>
#include <Blue/Memory/Api.h>

namespace Blue
{
class BLUE_MEMORY_API HeapAllocator
{
public:
  AllocationResult Allocate( const AllocationRequest& request );
  AllocationResult Reallocate( void* pointer, Size oldSize, const AllocationRequest& request );
  void Free( const AllocationFreeRequest& request );
};
} // namespace Blue

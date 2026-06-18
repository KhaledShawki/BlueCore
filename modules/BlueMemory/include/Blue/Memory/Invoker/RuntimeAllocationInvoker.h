#pragma once

#include <Blue/Memory/Allocation/AllocationFreeRequest.h>
#include <Blue/Memory/Allocation/AllocationValidation.h>
#include <Blue/Memory/Api.h>
#include <Blue/Memory/Proxy/RuntimeAllocationProxy.h>

namespace Blue
{
struct BLUE_MEMORY_API RuntimeAllocationInvoker
{
  static void* TryAllocate( const AllocationRequest& request ) noexcept;
  static void* Allocate( const AllocationRequest& request ) noexcept;
  static void Free( const AllocationFreeRequest& request ) noexcept;
};

BLUE_MEMORY_API void* BlueTryAllocate( const AllocationRequest& request ) noexcept;
BLUE_MEMORY_API void* BlueAllocate( const AllocationRequest& request ) noexcept;
BLUE_MEMORY_API void BlueFree( const AllocationFreeRequest& request ) noexcept;
BLUE_MEMORY_API void BlueFree( void* pointer, Size size, Size alignment, MemoryPoolId pool ) noexcept;
} // namespace Blue

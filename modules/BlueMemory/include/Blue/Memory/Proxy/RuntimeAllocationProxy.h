#pragma once

#include <Blue/Memory/Allocation/AllocationFreeRequest.h>
#include <Blue/Memory/Allocation/AllocationValidation.h>
#include <Blue/Memory/Api.h>
#include <Blue/Memory/Proxy/AllocatorProxy.h>

namespace Blue
{
struct BLUE_MEMORY_API RuntimeAllocationProxy
{
	static void* Allocate( const AllocationRequest& request ) noexcept;
	static void Free( const AllocationFreeRequest& request ) noexcept;
};
} // namespace Blue

#pragma once

#include <Blue/Memory/Allocator.h>
#include <Blue/Memory/Api.h>

namespace Blue
{
struct LinearAllocator
{
  Byte* Start;
  Byte* Current;
  Byte* End;
  AllocationTag DefaultTag;
};

BLUE_MEMORY_API void InitializeLinearAllocator( LinearAllocator& allocator,
                                                void* memory,
                                                Size size,
                                                AllocationTag defaultTag );
BLUE_MEMORY_API void ResetLinearAllocator( LinearAllocator& allocator );
BLUE_MEMORY_API Allocator MakeLinearAllocator( LinearAllocator& allocator );
} // namespace Blue

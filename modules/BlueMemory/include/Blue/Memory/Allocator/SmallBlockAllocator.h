#pragma once

#include <Blue/Memory/Api.h>
#include <Blue/Memory/MemoryUnits.h>
#include <Blue/System/Types.h>

namespace Blue
{
constexpr Size BlueSmallBlockMinSize = 16;
constexpr Size BlueSmallBlockMaxSize = 512;
constexpr Size BlueSmallBlockClassCount = 6;
constexpr Size BlueSmallBlockSlabSize = BLUE_KB( 64 );

struct SmallBlockAllocatorStats
{
  Uint64 SlabCount = 0;
  Uint64 SlabBytes = 0;
  Uint64 AllocateCount = 0;
  Uint64 FreeCount = 0;
  Uint64 RefillCount = 0;
  Uint64 FailedRefillCount = 0;
};

BLUE_MEMORY_API Bool InitializeSmallBlockAllocator( ) noexcept;
BLUE_MEMORY_API void ShutdownSmallBlockAllocator( ) noexcept;
BLUE_MEMORY_API Bool IsSmallBlockAllocationSupported( Size size, Size alignment ) noexcept;
BLUE_MEMORY_API Size GetSmallBlockClassSize( Size size, Size alignment ) noexcept;
BLUE_MEMORY_API void* AllocateSmallBlock( Size size, Size alignment ) noexcept;
BLUE_MEMORY_API void FreeSmallBlock( void* pointer, Size size, Size alignment ) noexcept;
BLUE_MEMORY_API SmallBlockAllocatorStats GetSmallBlockAllocatorStats( ) noexcept;
} // namespace Blue

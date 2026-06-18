#pragma once

#include <Blue/Memory/Api.h>
#include <Blue/System/Types.h>

namespace Blue
{
struct VirtualMemoryBlock
{
  void* ReservedBase;
  Size ReservedSize;
  void* CommittedBase;
  Size CommittedSize;
  Size PageSize;
  Uint32 Flags;
};

BLUE_MEMORY_API bool ReserveVirtualMemory( VirtualMemoryBlock& block, Size size );
BLUE_MEMORY_API bool CommitVirtualMemory( VirtualMemoryBlock& block, Size offset, Size size );
BLUE_MEMORY_API bool DecommitVirtualMemory( VirtualMemoryBlock& block, Size offset, Size size );
BLUE_MEMORY_API void ReleaseVirtualMemory( VirtualMemoryBlock& block );
} // namespace Blue

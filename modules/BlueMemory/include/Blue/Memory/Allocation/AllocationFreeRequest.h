#pragma once

#include <Blue/Memory/AllocationTag.h>
#include <Blue/Memory/Pool/MemoryPoolId.h>
#include <Blue/System/Types.h>

namespace Blue
{
struct AllocationFreeRequest
{
  void* Pointer = nullptr;
  Size ByteSize = 0;
  Size Alignment = 0;
  MemoryPoolId Pool = MemoryPoolId::System;
  AllocationTag Tag = AllocationTag::Unknown;
};
} // namespace Blue

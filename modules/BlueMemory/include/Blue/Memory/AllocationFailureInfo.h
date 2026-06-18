#pragma once

#include <Blue/Memory/AllocationFailureReason.h>
#include <Blue/Memory/AllocationTag.h>
#include <Blue/Memory/AllocatorKind.h>
#include <Blue/Memory/Pool/MemoryPoolId.h>
#include <Blue/System/SourceLocation.h>
#include <Blue/System/Types.h>

namespace Blue
{
struct AllocationFailureInfo
{
  AllocationFailureReason Reason = AllocationFailureReason::None;
  MemoryPoolId Pool = MemoryPoolId::System;
  AllocatorKind Allocator = AllocatorKind::Default;
  AllocationTag Tag = AllocationTag::Unknown;
  Size RequestedSize = 0;
  Size RequestedAlignment = 0;
  Size PoolBudgetBytes = 0;
  Size PoolCurrentBytes = 0;
  Size PoolPeakBytes = 0;
  SourceLocation Location = { };
};

using AllocationFailureHandler = void ( * )( const AllocationFailureInfo& info ) noexcept;
} // namespace Blue

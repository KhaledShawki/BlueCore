#pragma once

#include <Blue/Memory/AllocatorKind.h>
#include <Blue/Memory/MemoryMetricsMode.h>
#include <Blue/Memory/Pool/MemoryPoolId.h>
#include <Blue/System/Types.h>

namespace Blue
{
struct MemoryPoolDesc
{
  MemoryPoolId Id = MemoryPoolId::System;
  const Char* Name = "System";
  Size BudgetBytes = 0;
  AllocatorKind Allocator = AllocatorKind::Default;
  MemoryMetricsMode Metrics = MemoryMetricsMode::Counters;
  Bool EnableOomReports = true;
};
} // namespace Blue

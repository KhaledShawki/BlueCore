#pragma once

#include <Blue/System/Types.h>

namespace Blue
{
enum class MemoryMetricsMode : Uint8
{
  Disabled,
  Counters,
  ThreadCounters,
  Tracking,
  FullDiagnostics
};
} // namespace Blue

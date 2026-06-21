// Copyright (c) Khaled Shawki. All rights reserved.

#pragma once

#include <Blue/Memory/AllocationFailureReason.h>
#include <Blue/System/Types.h>

namespace Blue
{
enum class OomPolicy : Uint8
{
  ReturnNull,
  ReportAndReturnNull,
  Fatal,
  Callback
};

using OomCallback = void ( * )( AllocationFailureReason reason, Size size ) noexcept;

struct BlueMemorySettings
{
  OomPolicy OutOfMemoryPolicy = OomPolicy::ReturnNull;
  OomCallback OutOfMemoryCallback = nullptr;

  Bool EnableRuntimePoolBudgets = true;
  Bool EnableRuntimeMetricsFlush = true;
  Bool EnableRuntimeLeakReportOnShutdown = true;

  Size MetricsFlushAllocationInterval = 4096;
  Size MetricsFlushByteThreshold = 1024 * 1024;
};
} // namespace Blue

#pragma once

#include <Blue/Memory/AllocationFailureInfo.h>
#include <Blue/Memory/Api.h>
#include <Blue/Memory/Oom/OomReport.h>
#include <Blue/System/Types.h>

namespace Blue
{
constexpr Size BlueDefaultOomReportCapacity = 128;

BLUE_MEMORY_API void ConfigureOomReporter( OomReport* externalBuffer, Size capacity ) noexcept;
BLUE_MEMORY_API void ClearOomReports( ) noexcept;
BLUE_MEMORY_API void RecordOomReport( const AllocationFailureInfo& info ) noexcept;
BLUE_MEMORY_API Size CaptureOomReports( OomReport* output, Size capacity ) noexcept;
} // namespace Blue

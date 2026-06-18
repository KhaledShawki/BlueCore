#pragma once

#include <Blue/System/Api.h>
#include <Blue/System/Log/LogSink.h>
#include <Blue/System/Types.h>

namespace Blue
{
constexpr Uint32 MaxLogSinkCount = 16;

struct LoggerStateSnapshot final
{
  Bool Initialized = false;
  Uint32 SinkCount = 0;
  Uint64 WrittenEventCount = 0;
  Uint64 DroppedEventCount = 0;
};

BLUE_SYSTEM_API Bool InitializeLogger( ) noexcept;
BLUE_SYSTEM_API void ShutdownLogger( ) noexcept;

BLUE_SYSTEM_API Bool RegisterLogSink( const LogSink& sink ) noexcept;
BLUE_SYSTEM_API void ClearLogSinks( ) noexcept;
BLUE_SYSTEM_API void FlushLogger( ) noexcept;

BLUE_SYSTEM_API Bool ShouldLog( const LogCategory& category, LogLevel level ) noexcept;
BLUE_SYSTEM_API void WriteLog( LogEvent event ) noexcept;

BLUE_SYSTEM_API LoggerStateSnapshot GetLoggerStateSnapshot( ) noexcept;
} // namespace Blue

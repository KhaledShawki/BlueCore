#pragma once

#include <Blue/System/Api.h>
#include <Blue/System/Log/LogEvent.h>
#include <Blue/System/Types.h>

namespace Blue
{
using LogSinkWriteFn = void ( * )( void* context, const LogEvent& event );
using LogSinkFlushFn = void ( * )( void* context );

struct LogSink final
{
  void* Context = nullptr;
  LogSinkWriteFn Write = nullptr;
  LogSinkFlushFn Flush = nullptr;
  LogLevel MinimumLevel = LogLevel::Trace;
};

BLUE_SYSTEM_API LogSink CreateConsoleLogSink( LogLevel minimumLevel = LogLevel::Trace ) noexcept;

inline LogSink MakeConsoleLogSink( LogLevel minimumLevel = LogLevel::Trace ) noexcept
{
  return CreateConsoleLogSink( minimumLevel );
}
} // namespace Blue

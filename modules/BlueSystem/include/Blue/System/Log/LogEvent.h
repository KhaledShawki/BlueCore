#pragma once

#include <Blue/System/Log/LogCategory.h>
#include <Blue/System/Threading/ThreadTypes.h>
#include <Blue/System/Types.h>

namespace Blue
{
struct LogEvent final
{
  LogLevel Level = LogLevel::Info;
  const LogCategory* Category = nullptr;
  const Char* Message = nullptr;
  const Char* File = nullptr;
  const Char* Function = nullptr;
  Uint32 Line = 0;
  ThreadId Thread = 0;
  Uint64 Sequence = 0;
};
} // namespace Blue

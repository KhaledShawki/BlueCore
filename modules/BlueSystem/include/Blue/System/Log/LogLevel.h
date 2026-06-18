#pragma once

#include <Blue/System/Types.h>

namespace Blue
{
enum class LogLevel : Uint8
{
  Trace = 0,
  Debug,
  Info,
  Warning,
  Error,
  Fatal,
};

const Char* GetLogLevelName( LogLevel level ) noexcept;
} // namespace Blue

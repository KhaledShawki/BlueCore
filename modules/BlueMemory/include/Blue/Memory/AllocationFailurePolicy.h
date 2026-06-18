#pragma once

#include <Blue/System/Types.h>

namespace Blue
{
enum class AllocationFailurePolicy : Uint8
{
  ReturnNull,
  Abort,
  CallHandlerThenAbort
};
} // namespace Blue

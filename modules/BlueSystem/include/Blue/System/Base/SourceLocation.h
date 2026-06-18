#pragma once

#include <Blue/System/Compiler.h>
#include <Blue/System/Types.h>

namespace Blue
{
struct SourceLocation
{
  const Char* File;
  const Char* Function;
  Uint32 Line;
};
} // namespace Blue

#define BLUE_SOURCE_LOCATION( )                                                                                        \
  Blue::SourceLocation                                                                                                 \
  {                                                                                                                    \
    __FILE__, BLUE_FUNCTION_NAME, static_cast< Blue::Uint32 >( __LINE__ )                                              \
  }

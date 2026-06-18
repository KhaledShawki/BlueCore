#pragma once

#include <Blue/Memory/Api.h>
#include <Blue/System/Types.h>

namespace Blue
{
struct BLUE_MEMORY_API SystemMemoryBackend
{
  static void* Allocate( Size size, Size alignment ) noexcept;
  static void* Reallocate( void* pointer, Size oldSize, Size newSize, Size alignment ) noexcept;
  static void Free( void* pointer, Size size, Size alignment ) noexcept;
};
} // namespace Blue

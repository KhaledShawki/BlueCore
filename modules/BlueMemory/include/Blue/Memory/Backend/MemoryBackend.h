// Copyright (c) Khaled Shawki. All rights reserved.

#pragma once

#include <Blue/Memory/Api.h>
#include <Blue/System/Types.h>

namespace Blue
{

enum class MemoryBackendKind
{
  System,
  Mimalloc
};

struct BLUE_MEMORY_API MemoryBackend
{
  static void* Allocate( Size size, Size alignment ) noexcept;
  static void* Reallocate( void* pointer, Size oldSize, Size newSize, Size alignment ) noexcept;
  static void Free( void* pointer, Size size, Size alignment ) noexcept;

  static MemoryBackendKind GetKind( ) noexcept;
  static const Char* GetName( ) noexcept;
};
} // namespace Blue

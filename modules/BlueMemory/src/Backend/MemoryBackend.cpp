// Copyright (c) Khaled Shawki. All rights reserved.
#include <Blue/Memory/Backend/MemoryBackend.h>

#include "Pch.h"

#ifndef BLUE_MEMORY_USE_MIMALLOC
#  define BLUE_MEMORY_USE_MIMALLOC 0
#endif

#if BLUE_MEMORY_USE_MIMALLOC
#  include "MimallocMemoryBackend.h"
#else
#  include "SystemMemoryBackend.h"
#endif

namespace Blue
{
void* MemoryBackend::Allocate( Size size, Size alignment ) noexcept
{
#if BLUE_MEMORY_USE_MIMALLOC
  return MimallocMemoryBackend::Allocate( size, alignment );
#else
  return SystemMemoryBackend::Allocate( size, alignment );
#endif
}

void* MemoryBackend::Reallocate( void* pointer, Size oldSize, Size newSize, Size alignment ) noexcept
{
#if BLUE_MEMORY_USE_MIMALLOC
  return MimallocMemoryBackend::Reallocate( pointer, oldSize, newSize, alignment );
#else
  return SystemMemoryBackend::Reallocate( pointer, oldSize, newSize, alignment );
#endif
}

void MemoryBackend::Free( void* pointer, Size size, Size alignment ) noexcept
{
#if BLUE_MEMORY_USE_MIMALLOC
  MimallocMemoryBackend::Free( pointer, size, alignment );
#else
  SystemMemoryBackend::Free( pointer, size, alignment );
#endif
}
} // namespace Blue

// Copyright (c) Khaled Shawki. All rights reserved.

#include <Blue/Memory/Backend/MemoryBackend.h>
#include <Blue/Memory/Backend/SystemMemoryBackend.h>

#include "Pch.h"

namespace Blue
{
void* MemoryBackend::Allocate( Size size, Size alignment ) noexcept
{
  return SystemMemoryBackend::Allocate( size, alignment );
}

void* MemoryBackend::Reallocate( void* pointer, Size oldSize, Size newSize, Size alignment ) noexcept
{
  return SystemMemoryBackend::Reallocate( pointer, oldSize, newSize, alignment );
}

void MemoryBackend::Free( void* pointer, Size size, Size alignment ) noexcept
{
  SystemMemoryBackend::Free( pointer, size, alignment );
}
} // namespace Blue

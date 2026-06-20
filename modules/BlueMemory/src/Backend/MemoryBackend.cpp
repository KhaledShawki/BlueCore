// Copyright (c) Khaled Shawki. All rights reserved.

#include "Pch.h"

#include <Blue/Memory/Backend/MemoryBackend.h>


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
  if ( size == 0 )
  {
    return nullptr;
  }

#if BLUE_MEMORY_USE_MIMALLOC
  return MimallocMemoryBackend::Allocate( size, alignment );
#else
  return SystemMemoryBackend::Allocate( size, alignment );
#endif
}

void* MemoryBackend::Reallocate( void* pointer, Size oldSize, Size newSize, Size alignment ) noexcept
{
  if ( pointer == nullptr )
  {
    return Allocate( newSize, alignment );
  }

  if ( newSize == 0 )
  {
    Free( pointer, oldSize, alignment );
    return nullptr;
  }

#if BLUE_MEMORY_USE_MIMALLOC
  return MimallocMemoryBackend::Reallocate( pointer, oldSize, newSize, alignment );
#else
  return SystemMemoryBackend::Reallocate( pointer, oldSize, newSize, alignment );
#endif
}

void MemoryBackend::Free( void* pointer, Size size, Size alignment ) noexcept
{
  if ( pointer == nullptr )
  {
    return;
  }

#if BLUE_MEMORY_USE_MIMALLOC
  MimallocMemoryBackend::Free( pointer, size, alignment );
#else
  SystemMemoryBackend::Free( pointer, size, alignment );
#endif
}

MemoryBackendKind MemoryBackend::GetKind( ) noexcept
{
#if BLUE_MEMORY_USE_MIMALLOC
  return MemoryBackendKind::Mimalloc;
#else
  return MemoryBackendKind::System;
#endif
}

const Char* MemoryBackend::GetName( ) noexcept
{
#if BLUE_MEMORY_USE_MIMALLOC
  return "mimalloc";
#else
  return "system";
#endif
}
} // namespace Blue

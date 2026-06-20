// Copyright (c) Khaled Shawki. All rights reserved.
#include "Pch.h"

#include "MimallocMemoryBackend.h"


#ifndef BLUE_MEMORY_USE_MIMALLOC
#  define BLUE_MEMORY_USE_MIMALLOC 0
#endif

#if BLUE_MEMORY_USE_MIMALLOC
#  if !__has_include( <mimalloc.h> )
#    error "BLUE_MEMORY_USE_MIMALLOC=1 but mimalloc.h was not found. Initialize third_party/mimalloc."
#  endif
#  include <mimalloc.h>

namespace Blue
{
void* MimallocMemoryBackend::Allocate( Size size, Size alignment ) noexcept
{
  return mi_malloc_aligned( size, alignment );
}

void* MimallocMemoryBackend::Reallocate( void* pointer, Size, Size newSize, Size alignment ) noexcept
{
  return mi_realloc_aligned( pointer, newSize, alignment );
}

void MimallocMemoryBackend::Free( void* pointer, Size, Size ) noexcept
{
  mi_free( pointer );
}
} // namespace Blue
#endif

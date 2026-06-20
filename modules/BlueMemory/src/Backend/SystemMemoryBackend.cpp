// Copyright (c) Khaled Shawki. All rights reserved.

#include "Pch.h"

#include <cstddef>
#include <cstdlib>
#include <cstring>

#include "Backend/SystemMemoryBackend.h"


#if defined( _WIN32 )
#  include <malloc.h>
#endif

namespace
{
constexpr bool RequiresAlignedAllocation( Blue::Size alignment ) noexcept
{
  return alignment > alignof( std::max_align_t );
}
} // namespace

namespace Blue
{
void* SystemMemoryBackend::Allocate( Size size, Size alignment ) noexcept
{
  if ( !RequiresAlignedAllocation( alignment ) )
  {
    return std::malloc( size );
  }

#if defined( _WIN32 )
  return _aligned_malloc( size, alignment );
#else
  void* pointer = nullptr;
  if ( posix_memalign( &pointer, alignment, size ) != 0 )
  {
    return nullptr;
  }

  return pointer;
#endif
}

void* SystemMemoryBackend::Reallocate( void* pointer, Size oldSize, Size newSize, Size alignment ) noexcept
{
  if ( !RequiresAlignedAllocation( alignment ) )
  {
    return std::realloc( pointer, newSize );
  }

#if defined( _WIN32 )
  return _aligned_realloc( pointer, newSize, alignment );
#else
  void* newPointer = Allocate( newSize, alignment );
  if ( newPointer == nullptr )
  {
    return nullptr;
  }

  const Size bytesToCopy = oldSize < newSize ? oldSize : newSize;
  std::memcpy( newPointer, pointer, bytesToCopy );

  Free( pointer, oldSize, alignment );
  return newPointer;
#endif
}

void SystemMemoryBackend::Free( void* pointer, Size, Size alignment ) noexcept
{
#if defined( _WIN32 )
  if ( RequiresAlignedAllocation( alignment ) )
  {
    _aligned_free( pointer );
    return;
  }
#else
  BLUE_UNUSED( alignment );
#endif

  std::free( pointer );
}
} // namespace Blue

// Copyright (c) Khaled Shawki. All rights reserved.
#include "Backend/SystemMemoryBackend.h"

#include <Blue/System/Base/Alignment.h>

#include <cstddef>
#include <cstdlib>
#include <cstring>

#include "Pch.h"

#if defined( _WIN32 )
#  include <malloc.h>
#endif

namespace
{
static_assert( Blue::IsPowerOfTwo( alignof( std::max_align_t ) ) );

constexpr Blue::Size NormalizeAlignment( Blue::Size alignment ) noexcept
{
  constexpr Blue::Size minimumAlignment = alignof( std::max_align_t );
  return alignment < minimumAlignment ? minimumAlignment : alignment;
}

constexpr bool RequiresAlignedAllocation( Blue::Size normalizedAlignment ) noexcept
{
  return normalizedAlignment > alignof( std::max_align_t );
}
} // namespace

namespace Blue
{
void* SystemMemoryBackend::Allocate( Size size, Size alignment ) noexcept
{
  if ( size == 0 )
  {
    return nullptr;
  }

  alignment = NormalizeAlignment( alignment );

  if ( !IsPowerOfTwo( alignment ) )
  {
    return nullptr;
  }

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
  if ( pointer == nullptr )
  {
    return Allocate( newSize, alignment );
  }

  if ( newSize == 0 )
  {
    Free( pointer, oldSize, alignment );
    return nullptr;
  }

  alignment = NormalizeAlignment( alignment );

  if ( !IsPowerOfTwo( alignment ) )
  {
    return nullptr;
  }

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
  if ( pointer == nullptr )
  {
    return;
  }

  alignment = NormalizeAlignment( alignment );

#if defined( _WIN32 )
  if ( RequiresAlignedAllocation( alignment ) )
  {
    _aligned_free( pointer );
    return;
  }
#endif

  std::free( pointer );
}
} // namespace Blue

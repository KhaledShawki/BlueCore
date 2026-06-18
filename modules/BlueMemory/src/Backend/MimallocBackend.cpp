#include <Blue/System/Types.h>

#include <stdlib.h>
#include <string.h>
#if defined( _MSC_VER )
#  include <malloc.h>
#endif

#if BLUE_MEMORY_USE_MIMALLOC
#  if __has_include( <mimalloc.h>)
#    include <mimalloc.h>
#    define BLUE_MIMALLOC_AVAILABLE 1
#  else
#    define BLUE_MIMALLOC_AVAILABLE 0
#  endif
#else
#  define BLUE_MIMALLOC_AVAILABLE 0
#endif

namespace Blue::Backend
{
void* BackendAllocate( Size size, Size alignment );
void* BackendReallocate( void* pointer, Size oldSize, Size newSize, Size alignment );
void BackendFree( void* pointer );

void* BackendAllocate( Size size, Size alignment )
{
#if BLUE_MIMALLOC_AVAILABLE
  return mi_malloc_aligned( size, alignment );
#else
#  if defined( _MSC_VER )
  return _aligned_malloc( size, alignment );
#  else
  if ( alignment < sizeof( void* ) )
  {
    alignment = sizeof( void* );
  }

  void* pointer = nullptr;
  if ( posix_memalign( &pointer, alignment, size ) != 0 )
  {
    return nullptr;
  }
  return pointer;
#  endif
#endif
}

void* BackendReallocate( void* pointer, Size oldSize, Size newSize, Size alignment )
{
#if BLUE_MIMALLOC_AVAILABLE
  static_cast< void >( oldSize );
  return mi_realloc_aligned( pointer, newSize, alignment );
#else
  if ( !pointer )
  {
    return BackendAllocate( newSize, alignment );
  }

  void* newPointer = BackendAllocate( newSize, alignment );
  if ( !newPointer )
  {
    return nullptr;
  }

  const Size copySize = oldSize < newSize ? oldSize : newSize;
  if ( copySize > 0 )
  {
    memcpy( newPointer, pointer, copySize );
  }

  BackendFree( pointer );
  return newPointer;
#endif
}

void BackendFree( void* pointer )
{
#if BLUE_MIMALLOC_AVAILABLE
  mi_free( pointer );
#else
#  if defined( _MSC_VER )
  _aligned_free( pointer );
#  else
  free( pointer );
#  endif
#endif
}
} // namespace Blue::Backend

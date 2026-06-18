#pragma once

#include <Blue/Memory/Pool/MemoryPoolPolicy.h>
#include <Blue/Memory/Proxy/AllocatorProxy.h>

namespace Blue
{
template< typename T, MemoryPoolId Pool >
struct TypedAllocationProxy
{
  static void* Allocate( AllocationTag tag, SourceLocation location ) noexcept
  {
    using Policy = MemoryPoolPolicy< Pool >;
    return AllocatorProxy< Policy::Allocator, Pool >::Allocate( sizeof( T ), alignof( T ), tag, location );
  }

  static void Free( void* pointer ) noexcept
  {
    using Policy = MemoryPoolPolicy< Pool >;
    AllocatorProxy< Policy::Allocator, Pool >::Free( pointer, sizeof( T ), alignof( T ) );
  }
};
} // namespace Blue

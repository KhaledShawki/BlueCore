#pragma once

#include <Blue/Memory/Allocation/AllocationFreeRequest.h>
#include <Blue/Memory/AllocationFailurePolicy.h>
#include <Blue/Memory/AllocationFlags.h>
#include <Blue/Memory/AllocationTag.h>
#include <Blue/Memory/Pool/MemoryPoolId.h>
#include <Blue/System/Assert.h>
#include <Blue/System/Compiler.h>
#include <Blue/System/Types.h>

namespace Blue
{
struct AllocationRequest
{
  Size ByteSize = 0;
  Size Alignment = 0;
  AllocationTag Tag = AllocationTag::Unknown;
  const char* File = nullptr;
  const char* Function = nullptr;
  Uint32 Line = 0;
  MemoryPoolId Pool = MemoryPoolId::System;
  AllocationFailurePolicy FailurePolicy = AllocationFailurePolicy::ReturnNull;
  Uint32 Flags = AllocationFlag_None;
};

struct AllocationResult
{
  void* Pointer;
  Size ByteSize;
};

using AllocateFn = AllocationResult ( * )( void* context, const AllocationRequest& request );
using ReallocateFn = AllocationResult ( * )( void* context,
                                             void* pointer,
                                             Size oldSize,
                                             const AllocationRequest& request );
using FreeFn = void ( * )( void* context, const AllocationFreeRequest& request );

struct Allocator
{
  void* Context;
  AllocateFn Allocate;
  ReallocateFn Reallocate;
  FreeFn Free;
};

BLUE_FORCE_INLINE bool IsValidAllocator( const Allocator& allocator )
{
  return allocator.Allocate != nullptr && allocator.Free != nullptr;
}

BLUE_FORCE_INLINE AllocationResult Allocate( const Allocator& allocator, const AllocationRequest& request )
{
  BLUE_ASSERT( IsValidAllocator( allocator ) );
  BLUE_ASSERT( request.ByteSize > 0 );
  BLUE_ASSERT( request.Alignment > 0 );
  return allocator.Allocate( allocator.Context, request );
}

BLUE_FORCE_INLINE AllocationResult Reallocate( const Allocator& allocator,
                                               void* pointer,
                                               Size oldSize,
                                               const AllocationRequest& request )
{
  BLUE_ASSERT( IsValidAllocator( allocator ) );
  if ( allocator.Reallocate )
  {
    return allocator.Reallocate( allocator.Context, pointer, oldSize, request );
  }
  return { nullptr, 0 };
}

BLUE_FORCE_INLINE void Free( const Allocator& allocator, const AllocationFreeRequest& request )
{
  if ( !request.Pointer )
  {
    return;
  }

  BLUE_ASSERT( IsValidAllocator( allocator ) );
  allocator.Free( allocator.Context, request );
}

BLUE_FORCE_INLINE void Free( const Allocator& allocator, void* pointer, Size size, Size alignment )
{
  Free( allocator, AllocationFreeRequest{ pointer, size, alignment, MemoryPoolId::System, AllocationTag::Unknown } );
}
} // namespace Blue

#define BLUE_ALLOCATION_REQUEST( size, alignment, tag )                                                                \
  Blue::AllocationRequest                                                                                              \
  {                                                                                                                    \
    size, alignment, tag, __FILE__, __func__, static_cast< Blue::Uint32 >( __LINE__ )                                  \
  }

#define BLUE_POOL_ALLOCATION_REQUEST( size, alignment, tag, pool )                                                     \
  Blue::AllocationRequest                                                                                              \
  {                                                                                                                    \
    size, alignment, tag, __FILE__, __func__, static_cast< Blue::Uint32 >( __LINE__ ), pool                            \
  }

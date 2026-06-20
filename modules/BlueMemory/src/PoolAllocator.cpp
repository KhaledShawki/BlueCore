// Copyright (c) Khaled Shawki. All rights reserved.

#include "Pch.h"

#include <Blue/Memory/PoolAllocator.h>
#include <Blue/System/Alignment.h>


namespace Blue
{
PoolAllocator::PoolAllocator( )
    : m_Block{ }
    , m_FreeList( nullptr )
    , m_ObjectSize( 0 )
    , m_ObjectStride( 0 )
    , m_ObjectAlignment( 0 )
    , m_Capacity( 0 )
    , m_UsedCount( 0 )
    , m_BackingAllocator{ }
{}

namespace
{
Size GetPoolSlotStride( Size objectSize, Size objectAlignment )
{
  const Size minimumSlotSize = objectSize < sizeof( void* ) ? sizeof( void* ) : objectSize;
  return AlignUp( minimumSlotSize, objectAlignment );
}
} // namespace

bool PoolAllocator::Initialize( const PoolAllocatorDesc& desc )
{
  if ( desc.ObjectSize == 0 || desc.ObjectCount == 0 || !IsPowerOfTwo( desc.ObjectAlignment ) ||
       !IsValidAllocator( desc.BackingAllocator ) )
  {
    return false;
  }

  m_ObjectSize = desc.ObjectSize;
  m_ObjectAlignment = desc.ObjectAlignment;
  m_ObjectStride = GetPoolSlotStride( desc.ObjectSize, desc.ObjectAlignment );
  m_Capacity = desc.ObjectCount;
  m_BackingAllocator = desc.BackingAllocator;

  AllocationResult result =
    Blue::Allocate( m_BackingAllocator,
                    BLUE_ALLOCATION_REQUEST( m_ObjectStride * m_Capacity, m_ObjectAlignment, desc.Tag ) );

  if ( !result.Pointer )
  {
    return false;
  }

  m_Block = { result.Pointer, result.ByteSize, m_ObjectAlignment, desc.Tag, 0, MemoryBlockFlag_Owned };
  Byte* current = static_cast< Byte* >( m_Block.Base );

  for ( Size index = 0; index < m_Capacity - 1; ++index )
  {
    void** slot = reinterpret_cast< void** >( current + index * m_ObjectStride );
    *slot = current + ( index + 1 ) * m_ObjectStride;
  }

  void** lastSlot = reinterpret_cast< void** >( current + ( m_Capacity - 1 ) * m_ObjectStride );
  *lastSlot = nullptr;
  m_FreeList = current;
  m_UsedCount = 0;
  return true;
}

void PoolAllocator::Shutdown( )
{
  if ( m_Block.Base )
  {
    Blue::Free( m_BackingAllocator, m_Block.Base, m_Block.ByteSize, m_Block.Alignment );
  }
  *this = PoolAllocator( );
}

AllocationResult PoolAllocator::Allocate( const AllocationRequest& request )
{
  if ( !m_FreeList || request.ByteSize == 0 || request.ByteSize > m_ObjectSize ||
       request.Alignment > m_ObjectAlignment )
  {
    return { nullptr, 0 };
  }

  void* result = m_FreeList;
  m_FreeList = *reinterpret_cast< void** >( m_FreeList );
  ++m_UsedCount;
  return { result, m_ObjectSize };
}

AllocationResult PoolAllocator::Reallocate( void*, Size, const AllocationRequest& )
{
  return { nullptr, 0 };
}

void PoolAllocator::Free( void* pointer, Size, Size )
{
  if ( !pointer || !m_Block.Base || m_ObjectStride == 0 || m_UsedCount == 0 )
  {
    return;
  }

  const NativeUInt base = reinterpret_cast< NativeUInt >( m_Block.Base );
  const NativeUInt address = reinterpret_cast< NativeUInt >( pointer );
  if ( address < base || address >= base + m_Block.ByteSize )
  {
    return;
  }

  const Size offset = static_cast< Size >( address - base );
  if ( ( offset % m_ObjectStride ) != 0 )
  {
    return;
  }

  *reinterpret_cast< void** >( pointer ) = m_FreeList;
  m_FreeList = pointer;
  --m_UsedCount;
}

void PoolAllocator::Free( const AllocationFreeRequest& request )
{
  Free( request.Pointer, request.ByteSize, request.Alignment );
}

Size PoolAllocator::GetObjectSize( ) const
{
  return m_ObjectSize;
}

Size PoolAllocator::GetCapacity( ) const
{
  return m_Capacity;
}

Size PoolAllocator::GetUsedCount( ) const
{
  return m_UsedCount;
}

Size PoolAllocator::GetObjectStride( ) const
{
  return m_ObjectStride;
}
} // namespace Blue

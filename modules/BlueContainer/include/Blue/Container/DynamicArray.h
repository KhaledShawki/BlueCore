#pragma once

#include <Blue/Memory/Allocator.h>
#include <Blue/System/Assert.h>

#include <new>

namespace Blue
{
template< typename T >
class DynamicArray
{
  public:
  explicit DynamicArray( Allocator allocator )
      : m_Data( nullptr )
      , m_Size( 0 )
      , m_Capacity( 0 )
      , m_Allocator( allocator )
  {}

  DynamicArray( const DynamicArray& ) = delete;
  DynamicArray& operator=( const DynamicArray& ) = delete;

  DynamicArray( DynamicArray&& other ) noexcept
      : m_Data( other.m_Data )
      , m_Size( other.m_Size )
      , m_Capacity( other.m_Capacity )
      , m_Allocator( other.m_Allocator )
  {
    other.m_Data = nullptr;
    other.m_Size = 0;
    other.m_Capacity = 0;
    other.m_Allocator = { };
  }

  ~DynamicArray( )
  {
    Clear( );
    if ( m_Data )
    {
      Blue::Free( m_Allocator, m_Data, sizeof( T ) * m_Capacity, alignof( T ) );
    }
  }

  bool Reserve( Size capacity )
  {
    if ( capacity <= m_Capacity )
    {
      return true;
    }

    AllocationResult result =
      Blue::Allocate( m_Allocator,
                      BLUE_ALLOCATION_REQUEST( sizeof( T ) * capacity, alignof( T ), AllocationTag::Container ) );

    if ( !result.Pointer )
    {
      return false;
    }

    T* newData = static_cast< T* >( result.Pointer );
    for ( Size index = 0; index < m_Size; ++index )
    {
      new ( &newData[ index ] ) T( static_cast< T&& >( m_Data[ index ] ) );
      m_Data[ index ].~T( );
    }

    if ( m_Data )
    {
      Blue::Free( m_Allocator, m_Data, sizeof( T ) * m_Capacity, alignof( T ) );
    }

    m_Data = newData;
    m_Capacity = capacity;
    return true;
  }

  bool PushBack( const T& value )
  {
    if ( m_Size == m_Capacity && !Reserve( m_Capacity == 0 ? 8 : m_Capacity * 2 ) )
    {
      return false;
    }

    new ( &m_Data[ m_Size ] ) T( value );
    ++m_Size;
    return true;
  }

  void Clear( )
  {
    for ( Size index = 0; index < m_Size; ++index )
    {
      m_Data[ index ].~T( );
    }
    m_Size = 0;
  }

  T& operator[]( Size index )
  {
    BLUE_ASSERT( index < m_Size );
    return m_Data[ index ];
  }

  const T& operator[]( Size index ) const
  {
    BLUE_ASSERT( index < m_Size );
    return m_Data[ index ];
  }

  T* Data( ) { return m_Data; }

  const T* Data( ) const { return m_Data; }

  Size GetSize( ) const { return m_Size; }

  Size GetCapacity( ) const { return m_Capacity; }

  bool Empty( ) const { return m_Size == 0; }

  private:
  T* m_Data;
  Size m_Size;
  Size m_Capacity;
  Allocator m_Allocator;
};
} // namespace Blue

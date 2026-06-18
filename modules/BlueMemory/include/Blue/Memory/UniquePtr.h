#pragma once

#include <Blue/Memory/Allocator.h>

#include <new>

namespace Blue
{
template< typename T >
class UniquePtr
{
  public:
  UniquePtr( )
      : m_Pointer( nullptr )
      , m_Allocator{ }
  {}

  UniquePtr( T* pointer, Allocator allocator )
      : m_Pointer( pointer )
      , m_Allocator( allocator )
  {}

  UniquePtr( const UniquePtr& ) = delete;
  UniquePtr& operator=( const UniquePtr& ) = delete;

  UniquePtr( UniquePtr&& other ) noexcept
      : m_Pointer( other.m_Pointer )
      , m_Allocator( other.m_Allocator )
  {
    other.m_Pointer = nullptr;
    other.m_Allocator = { };
  }

  ~UniquePtr( ) { Reset( ); }

  void Reset( )
  {
    if ( m_Pointer )
    {
      m_Pointer->~T( );
      Blue::Free( m_Allocator, m_Pointer, sizeof( T ), alignof( T ) );
      m_Pointer = nullptr;
    }
  }

  T* Get( ) const { return m_Pointer; }

  T& operator*( ) const { return *m_Pointer; }

  T* operator->( ) const { return m_Pointer; }

  explicit operator bool( ) const { return m_Pointer != nullptr; }

  private:
  T* m_Pointer;
  Allocator m_Allocator;
};

template< typename T, typename... TArgs >
UniquePtr< T > MakeUnique( Allocator allocator, AllocationTag tag, TArgs&&... args )
{
  AllocationResult result = Blue::Allocate( allocator, BLUE_ALLOCATION_REQUEST( sizeof( T ), alignof( T ), tag ) );
  if ( !result.Pointer )
  {
    return UniquePtr< T >( );
  }

  T* object = new ( result.Pointer ) T( static_cast< TArgs&& >( args )... );
  return UniquePtr< T >( object, allocator );
}
} // namespace Blue

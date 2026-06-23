#pragma once

#include <Blue/System/Assert.h>
#include <Blue/System/Types.h>

namespace Blue
{
template< typename T >
class Span
{
public:
  Span( )
      : m_Data( nullptr )
      , m_Size( 0 )
  {}

  Span( T* data, Size size )
      : m_Data( data )
      , m_Size( size )
  {}

  T* Data( ) const { return m_Data; }

  Size GetSize( ) const { return m_Size; }

  bool Empty( ) const { return m_Size == 0; }

  T& operator[]( Size index ) const
  {
    BLUE_ASSERT( index < m_Size );
    return m_Data[ index ];
  }

private:
  T* m_Data;
  Size m_Size;
};
} // namespace Blue

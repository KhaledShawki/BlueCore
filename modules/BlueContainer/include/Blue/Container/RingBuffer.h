#pragma once

#include <Blue/System/Assert.h>
#include <Blue/System/Types.h>

namespace Blue
{
template< typename T, Size CapacityValue >
class FixedRingBuffer
{
  public:
  bool Push( const T& value )
  {
    if ( m_Count == CapacityValue )
    {
      return false;
    }

    m_Items[ m_Tail ] = value;
    m_Tail = ( m_Tail + 1 ) % CapacityValue;
    ++m_Count;
    return true;
  }

  bool Pop( T& outValue )
  {
    if ( m_Count == 0 )
    {
      return false;
    }

    outValue = m_Items[ m_Head ];
    m_Head = ( m_Head + 1 ) % CapacityValue;
    --m_Count;
    return true;
  }

  Size Count( ) const { return m_Count; }

  bool Empty( ) const { return m_Count == 0; }

  bool Full( ) const { return m_Count == CapacityValue; }

  private:
  T m_Items[ CapacityValue ] = { };
  Size m_Head = 0;
  Size m_Tail = 0;
  Size m_Count = 0;
};
} // namespace Blue

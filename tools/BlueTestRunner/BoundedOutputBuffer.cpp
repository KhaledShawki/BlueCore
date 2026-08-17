#include "BoundedOutputBuffer.h"

#include <algorithm>
#include <cstring>
#include <limits>

namespace BlueTestRunner
{
namespace
{
constexpr std::string_view TruncationMarker = "\n[BlueRunTests] Captured output truncated.\n";
}

BoundedOutputBuffer::BoundedOutputBuffer( const std::size_t maximumBytes )
    : m_maximumBytes( maximumBytes )
    , m_headCapacity( maximumBytes / 2 )
    , m_tailCapacity( maximumBytes - m_headCapacity )
{}

void BoundedOutputBuffer::Append( const char* data, const std::size_t size )
{
  if ( data == nullptr || size == 0 )
  {
    return;
  }

  const auto maximum = std::numeric_limits< std::uint64_t >::max( );
  if ( size > maximum - m_producedByteCount )
  {
    m_producedByteCount = maximum;
  }
  else
  {
    m_producedByteCount += static_cast< std::uint64_t >( size );
  }

  std::size_t offset = 0;
  if ( m_head.size( ) < m_headCapacity )
  {
    const std::size_t copyCount = std::min( m_headCapacity - m_head.size( ), size );
    m_head.append( data, copyCount );
    offset += copyCount;
  }

  if ( offset < size )
  {
    AppendTail( data + offset, size - offset );
  }
}

void BoundedOutputBuffer::Append( const std::string_view data )
{
  Append( data.data( ), data.size( ) );
}

void BoundedOutputBuffer::AppendTail( const char* data, const std::size_t size )
{
  if ( m_tailCapacity == 0 || size == 0 )
  {
    return;
  }

  if ( size >= m_tailCapacity )
  {
    m_tail.assign( data + size - m_tailCapacity, data + size );
    m_tailStart = 0;
    m_tailSize = m_tailCapacity;
    return;
  }

  std::size_t offset = 0;
  if ( m_tailSize < m_tailCapacity )
  {
    const std::size_t copyCount = std::min( m_tailCapacity - m_tailSize, size );
    m_tail.insert( m_tail.end( ), data, data + copyCount );
    m_tailSize += copyCount;
    offset += copyCount;
  }

  while ( offset < size )
  {
    const std::size_t contiguous = std::min( size - offset, m_tailCapacity - m_tailStart );
    std::memcpy( m_tail.data( ) + m_tailStart, data + offset, contiguous );
    m_tailStart = ( m_tailStart + contiguous ) % m_tailCapacity;
    offset += contiguous;
  }
}

void BoundedOutputBuffer::AppendLinearizedTail( std::string& output ) const
{
  if ( m_tailSize == 0 )
  {
    return;
  }

  if ( m_tailSize < m_tailCapacity || m_tailStart == 0 )
  {
    output.append( m_tail.data( ), m_tailSize );
    return;
  }

  const std::size_t firstPartSize = m_tailCapacity - m_tailStart;
  output.append( m_tail.data( ) + m_tailStart, firstPartSize );
  output.append( m_tail.data( ), m_tailStart );
}

std::string BoundedOutputBuffer::BuildOutput( ) const
{
  std::string output;
  output.reserve( m_head.size( ) + m_tailSize + ( Truncated( ) ? TruncationMarker.size( ) : 0 ) );
  output.append( m_head );

  if ( Truncated( ) )
  {
    output.append( TruncationMarker );
  }

  AppendLinearizedTail( output );
  return output;
}
} // namespace BlueTestRunner

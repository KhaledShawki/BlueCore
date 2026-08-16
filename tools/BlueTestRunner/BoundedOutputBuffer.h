#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace BlueTestRunner
{
class BoundedOutputBuffer
{
public:
  explicit BoundedOutputBuffer( std::size_t maximumBytes );

  void Append( const char* data, std::size_t size );
  void Append( std::string_view data );

  [[nodiscard]] std::string BuildOutput( ) const;
  [[nodiscard]] std::uint64_t ProducedBytes( ) const { return m_producedByteCount; }
  [[nodiscard]] bool Truncated( ) const { return m_producedByteCount > m_maximumBytes; }

private:
  void AppendTail( const char* data, std::size_t size );
  void AppendLinearizedTail( std::string& output ) const;

  std::size_t m_maximumBytes = 0;
  std::size_t m_headCapacity = 0;
  std::size_t m_tailCapacity = 0;
  std::string m_head;
  std::vector< char > m_tail;
  std::size_t m_tailStart = 0;
  std::size_t m_tailSize = 0;
  std::uint64_t m_producedByteCount = 0;
};
} // namespace BlueTestRunner

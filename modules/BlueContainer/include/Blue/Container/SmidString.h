#pragma once

#include <Blue/Container/Api.h>
#include <Blue/Container/StringView.h>
#include <Blue/Memory/Allocator.h>

namespace Blue
{
class BLUE_CONTAINER_API SmidString
{
public:
  static constexpr Size InlineCapacity = 23;

  explicit SmidString( Allocator allocator );
  SmidString( const char* text, Allocator allocator );
  SmidString( const char* text, Size size, Allocator allocator );
  SmidString( const SmidString& other, Allocator allocator );
  SmidString( SmidString&& other ) noexcept;
  ~SmidString( );

  SmidString& operator=( SmidString&& other ) noexcept;

  const char* Data( ) const;
  char* Data( );
  Size GetSize( ) const;
  Size GetCapacity( ) const;
  bool Empty( ) const;
  bool IsInline( ) const;

  void Clear( );
  bool Reserve( Size newCapacity );
  bool Append( const char* text, Size size );
  StringView View( ) const;

private:
  union Storage
  {
    char Inline[ InlineCapacity + 1 ];
    char* Heap;
  };

  Storage m_Storage;
  Size m_Size;
  Size m_Capacity;
  Allocator m_Allocator;
  bool m_IsInline;
};
} // namespace Blue

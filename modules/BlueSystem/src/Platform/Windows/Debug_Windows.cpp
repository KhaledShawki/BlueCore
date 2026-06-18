#include <Blue/System/Debug.h>
#include <Blue/System/Platform/WindowsLean.h>
#include <Blue/System/Types.h>

namespace Blue
{
Bool IsDebuggerAttached( ) noexcept
{
  return IsDebuggerPresent( ) != 0;
}

void BreakIntoDebugger( ) noexcept
{
  __debugbreak( );
}

void WriteDebugOutput( const Char* message ) noexcept
{
  if ( !message )
  {
    return;
  }

  OutputDebugStringA( message );
}
} // namespace Blue

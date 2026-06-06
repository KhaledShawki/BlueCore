#pragma once

#include <Blue/System/Api.h>
#include <Blue/System/Types.h>

namespace Blue
{
BLUE_SYSTEM_API Bool IsDebuggerAttached( ) noexcept;
BLUE_SYSTEM_API void BreakIntoDebugger( ) noexcept;
BLUE_SYSTEM_API void WriteDebugOutput( const Char* message ) noexcept;
} // namespace Blue

#define BLUE_DEBUG_BREAK( )                                                                                            \
	do                                                                                                                 \
	{                                                                                                                  \
		Blue::BreakIntoDebugger( );                                                                                    \
	}                                                                                                                  \
	while ( false )

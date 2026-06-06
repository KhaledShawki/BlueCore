#pragma once

#include <Blue/System/Api.h>
#include <Blue/System/Compiler.h>
#include <Blue/System/Debug.h>
#include <Blue/System/SourceLocation.h>
#include <Blue/System/Types.h>

#ifndef BLUE_ENABLE_ASSERTS
#	if defined( BLUE_DEBUG ) || !defined( NDEBUG )
#		define BLUE_ENABLE_ASSERTS 1
#	else
#		define BLUE_ENABLE_ASSERTS 0
#	endif
#endif

namespace Blue
{
struct AssertContext
{
	const Char* Expression;
	const Char* Message;
	SourceLocation Location;
};

using AssertHandler = void ( * )( const AssertContext& context );

BLUE_SYSTEM_API void SetAssertHandler( AssertHandler handler );
BLUE_SYSTEM_API AssertHandler GetAssertHandler( );
BLUE_SYSTEM_API void ReportAssertion( const AssertContext& context );
BLUE_SYSTEM_API void ReportAssertion( const Char* expression, const Char* message, const Char* file, int line );
} // namespace Blue

#if BLUE_ENABLE_ASSERTS
#	define BLUE_ASSERT( expression )                                                                                  \
		do                                                                                                             \
		{                                                                                                              \
			if ( !( expression ) )                                                                                     \
			{                                                                                                          \
				Blue::AssertContext context = { #expression, nullptr, BLUE_SOURCE_LOCATION( ) };                       \
				Blue::ReportAssertion( context );                                                                      \
				BLUE_DEBUG_BREAK( );                                                                                   \
			}                                                                                                          \
		}                                                                                                              \
		while ( false )

#	define BLUE_ASSERT_MSG( expression, message )                                                                     \
		do                                                                                                             \
		{                                                                                                              \
			if ( !( expression ) )                                                                                     \
			{                                                                                                          \
				Blue::AssertContext context = { #expression, message, BLUE_SOURCE_LOCATION( ) };                       \
				Blue::ReportAssertion( context );                                                                      \
				BLUE_DEBUG_BREAK( );                                                                                   \
			}                                                                                                          \
		}                                                                                                              \
		while ( false )
#else
#	define BLUE_ASSERT( expression ) ( ( void ) 0 )
#	define BLUE_ASSERT_MSG( expression, message ) ( ( void ) 0 )
#endif

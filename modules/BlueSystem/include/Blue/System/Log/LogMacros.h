#pragma once

#include <Blue/System/Log/LogCategory.h>
#include <Blue/System/Log/Logger.h>
#include <Blue/System/SourceLocation.h>
#include <Blue/System/Threading/Thread.h>

#ifndef BLUE_ENABLE_LOGGING
#	if defined( BLUE_SHIPPING )
#		define BLUE_ENABLE_LOGGING 0
#	else
#		define BLUE_ENABLE_LOGGING 1
#	endif
#endif

#if BLUE_ENABLE_LOGGING
#	define BLUE_LOG( category, level, message )                                                                       \
		do                                                                                                             \
		{                                                                                                              \
			if ( Blue::ShouldLog( ( category ), ( level ) ) )                                                          \
			{                                                                                                          \
				Blue::LogEvent blueLogEvent{ };                                                                        \
				blueLogEvent.Level = ( level );                                                                        \
				blueLogEvent.Category = &( category );                                                                 \
				blueLogEvent.Message = ( message );                                                                    \
				const Blue::SourceLocation blueLogLocation = BLUE_SOURCE_LOCATION( );                                  \
				blueLogEvent.File = blueLogLocation.File;                                                              \
				blueLogEvent.Function = blueLogLocation.Function;                                                      \
				blueLogEvent.Line = blueLogLocation.Line;                                                              \
				blueLogEvent.Thread = Blue::GetCurrentThreadId( );                                                     \
				Blue::WriteLog( blueLogEvent );                                                                        \
			}                                                                                                          \
		}                                                                                                              \
		while ( false )
#else
#	define BLUE_LOG( category, level, message ) ( ( void ) 0 )
#endif

#define BLUE_LOG_TRACE( category, message ) BLUE_LOG( category, Blue::LogLevel::Trace, message )
#define BLUE_LOG_DEBUG( category, message ) BLUE_LOG( category, Blue::LogLevel::Debug, message )
#define BLUE_LOG_INFO( category, message ) BLUE_LOG( category, Blue::LogLevel::Info, message )
#define BLUE_LOG_WARNING( category, message ) BLUE_LOG( category, Blue::LogLevel::Warning, message )
#define BLUE_LOG_ERROR( category, message ) BLUE_LOG( category, Blue::LogLevel::Error, message )
#define BLUE_LOG_FATAL( category, message ) BLUE_LOG( category, Blue::LogLevel::Fatal, message )

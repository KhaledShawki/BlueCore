#pragma once

#define BLUE_PLATFORM_UNKNOWN 0
#define BLUE_PLATFORM_WINDOWS 1
#define BLUE_PLATFORM_LINUX 2
#define BLUE_PLATFORM_MACOS 3

#if defined( _WIN32 )
#	define BLUE_PLATFORM BLUE_PLATFORM_WINDOWS
#	define BLUE_PLATFORM_IS_WINDOWS 1
#else
#	define BLUE_PLATFORM_IS_WINDOWS 0
#endif

#if defined( __linux__ )
#	define BLUE_PLATFORM BLUE_PLATFORM_LINUX
#	define BLUE_PLATFORM_IS_LINUX 1
#else
#	define BLUE_PLATFORM_IS_LINUX 0
#endif

#if defined( __APPLE__ ) && defined( __MACH__ )
#	define BLUE_PLATFORM BLUE_PLATFORM_MACOS
#	define BLUE_PLATFORM_IS_MACOS 1
#else
#	define BLUE_PLATFORM_IS_MACOS 0
#endif

#if !defined( BLUE_PLATFORM )
#	define BLUE_PLATFORM BLUE_PLATFORM_UNKNOWN
#endif

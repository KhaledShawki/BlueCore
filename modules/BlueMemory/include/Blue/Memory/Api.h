#pragma once

// BlueMemory import/export annotations for static and shared builds.
// Shared builds are enabled by the build platform through BLUE_SHARED_LIBRARY.
// BLUE_BUILD_BLUE_MEMORY / BLUE_EXPORT_BlueMemory marks this module as the exporter.
#include <Blue/System/Api.h>

#if defined( BLUE_SHARED_LIBRARY )
#	if defined( BLUE_BUILD_BLUE_MEMORY ) || defined( BLUE_EXPORT_BlueMemory )
#		define BLUE_MEMORY_API BLUE_API_EXPORT
#		define BLUE_MEMORY_API_TEMPLATE
#	else
#		define BLUE_MEMORY_API BLUE_API_IMPORT
#		define BLUE_MEMORY_API_TEMPLATE extern
#	endif
#else
#	define BLUE_MEMORY_API
#	define BLUE_MEMORY_API_TEMPLATE extern
#endif

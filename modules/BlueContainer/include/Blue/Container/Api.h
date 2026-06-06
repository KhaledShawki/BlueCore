#pragma once

#include <Blue/System/Api.h>

#if defined( BLUE_SHARED_LIBRARY )
#	if defined( BLUE_BUILD_BLUE_CONTAINER ) || defined( BLUE_EXPORT_BlueContainer )
#		define BLUE_CONTAINER_API BLUE_API_EXPORT
#		define BLUE_CONTAINER_API_TEMPLATE
#	else
#		define BLUE_CONTAINER_API BLUE_API_IMPORT
#		define BLUE_CONTAINER_API_TEMPLATE extern
#	endif
#else
#	define BLUE_CONTAINER_API
#	define BLUE_CONTAINER_API_TEMPLATE extern
#endif

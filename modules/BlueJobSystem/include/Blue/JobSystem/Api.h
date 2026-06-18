#pragma once

#include <Blue/System/Api.h>

#if defined( BLUE_SHARED_LIBRARY )
#  if defined( BLUE_BUILD_BLUE_JOB_SYSTEM ) || defined( BLUE_EXPORT_BlueJobSystem )
#    define BLUE_JOB_SYSTEM_API BLUE_API_EXPORT
#    define BLUE_JOB_SYSTEM_API_TEMPLATE
#  else
#    define BLUE_JOB_SYSTEM_API BLUE_API_IMPORT
#    define BLUE_JOB_SYSTEM_API_TEMPLATE extern
#  endif
#else
#  define BLUE_JOB_SYSTEM_API
#  define BLUE_JOB_SYSTEM_API_TEMPLATE extern
#endif

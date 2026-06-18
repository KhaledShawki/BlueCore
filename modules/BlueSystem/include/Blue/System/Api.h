#pragma once

// BlueSystem import/export annotations for static and shared builds.
// BLUE_SHARED_LIBRARY enables dynamic linkage, while BLUE_BUILD_BLUE_SYSTEM
// marks this module as the exporter.
#if !defined( BLUE_SHARED_LIBRARY )
#  if defined( BLUE_DLL ) || defined( BLUE_WITH_DLL ) || defined( BLUE_WITH_SHARED_LIBRARY )
#    define BLUE_SHARED_LIBRARY 1
#  endif
#endif

#if defined( _WIN32 ) || defined( __CYGWIN__ )
#  define BLUE_API_EXPORT __declspec( dllexport )
#  define BLUE_API_IMPORT __declspec( dllimport )
#elif defined( __GNUC__ ) || defined( __clang__ )
#  define BLUE_API_EXPORT __attribute__( ( visibility( "default" ) ) )
#  define BLUE_API_IMPORT __attribute__( ( visibility( "default" ) ) )
#else
#  define BLUE_API_EXPORT
#  define BLUE_API_IMPORT
#endif

#if defined( BLUE_SHARED_LIBRARY )
#  if defined( BLUE_BUILD_BLUE_SYSTEM ) || defined( BLUE_EXPORT_BlueSystem )
#    define BLUE_SYSTEM_API BLUE_API_EXPORT
#    define BLUE_SYSTEM_API_TEMPLATE
#  else
#    define BLUE_SYSTEM_API BLUE_API_IMPORT
#    define BLUE_SYSTEM_API_TEMPLATE extern
#  endif
#else
#  define BLUE_SYSTEM_API
#  define BLUE_SYSTEM_API_TEMPLATE extern
#endif

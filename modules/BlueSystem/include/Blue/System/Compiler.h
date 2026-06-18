#pragma once

#define BLUE_COMPILER_UNKNOWN 0
#define BLUE_COMPILER_MSVC 1
#define BLUE_COMPILER_CLANG 2
#define BLUE_COMPILER_GCC 3

#if defined( _MSC_VER )
#  define BLUE_COMPILER BLUE_COMPILER_MSVC
#  define BLUE_COMPILER_IS_MSVC 1
#else
#  define BLUE_COMPILER_IS_MSVC 0
#endif

#if defined( __clang__ )
#  define BLUE_COMPILER_IS_CLANG 1
#else
#  define BLUE_COMPILER_IS_CLANG 0
#endif

#if defined( __GNUC__ ) && !defined( __clang__ )
#  define BLUE_COMPILER_IS_GCC 1
#else
#  define BLUE_COMPILER_IS_GCC 0
#endif

#if !defined( BLUE_COMPILER )
#  if BLUE_COMPILER_IS_CLANG
#    define BLUE_COMPILER BLUE_COMPILER_CLANG
#  elif BLUE_COMPILER_IS_GCC
#    define BLUE_COMPILER BLUE_COMPILER_GCC
#  else
#    define BLUE_COMPILER BLUE_COMPILER_UNKNOWN
#  endif
#endif

#if BLUE_COMPILER_IS_MSVC
#  define BLUE_FORCE_INLINE __forceinline
#  define BLUE_NO_INLINE __declspec( noinline )
#elif BLUE_COMPILER_IS_CLANG || BLUE_COMPILER_IS_GCC
#  define BLUE_FORCE_INLINE inline __attribute__( ( always_inline ) )
#  define BLUE_NO_INLINE __attribute__( ( noinline ) )
#else
#  define BLUE_FORCE_INLINE inline
#  define BLUE_NO_INLINE
#endif

#if BLUE_COMPILER_IS_MSVC
#  define BLUE_FUNCTION_NAME __FUNCSIG__
#elif BLUE_COMPILER_IS_CLANG || BLUE_COMPILER_IS_GCC
#  define BLUE_FUNCTION_NAME __PRETTY_FUNCTION__
#else
#  define BLUE_FUNCTION_NAME __func__
#endif

#ifndef BLUE_UNUSED
#  define BLUE_UNUSED( value ) static_cast< void >( value )
#endif

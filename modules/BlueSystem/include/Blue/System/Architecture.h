#pragma once

#define BLUE_ARCH_UNKNOWN 0
#define BLUE_ARCH_X86 1
#define BLUE_ARCH_X64 2
#define BLUE_ARCH_ARM 3
#define BLUE_ARCH_ARM64 4

#if defined( _M_X64 ) || defined( __x86_64__ )
#	define BLUE_ARCH BLUE_ARCH_X64
#elif defined( _M_IX86 ) || defined( __i386__ )
#	define BLUE_ARCH BLUE_ARCH_X86
#elif defined( _M_ARM64 ) || defined( __aarch64__ )
#	define BLUE_ARCH BLUE_ARCH_ARM64
#elif defined( _M_ARM ) || defined( __arm__ )
#	define BLUE_ARCH BLUE_ARCH_ARM
#else
#	define BLUE_ARCH BLUE_ARCH_UNKNOWN
#endif

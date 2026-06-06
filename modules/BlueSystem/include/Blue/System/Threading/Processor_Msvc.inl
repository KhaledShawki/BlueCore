#pragma once

#if BLUE_ARCH == BLUE_ARCH_X86 || BLUE_ARCH == BLUE_ARCH_X64
#	include <immintrin.h>
#endif

namespace Blue
{
BLUE_FORCE_INLINE void ProcessorPause( ) noexcept
{
#if BLUE_ARCH == BLUE_ARCH_X86 || BLUE_ARCH == BLUE_ARCH_X64
	_mm_pause( );
#elif BLUE_ARCH == BLUE_ARCH_ARM || BLUE_ARCH == BLUE_ARCH_ARM64
	__yield( );
#else
#endif
}
} // namespace Blue

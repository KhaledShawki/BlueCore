#pragma once

namespace Blue
{
BLUE_FORCE_INLINE void ProcessorPause( ) noexcept
{
#if BLUE_ARCH == BLUE_ARCH_X86 || BLUE_ARCH == BLUE_ARCH_X64
	__builtin_ia32_pause( );
#elif BLUE_ARCH == BLUE_ARCH_ARM || BLUE_ARCH == BLUE_ARCH_ARM64
	__asm__ __volatile__( "yield" ::: "memory" );
#else
#endif
}
} // namespace Blue

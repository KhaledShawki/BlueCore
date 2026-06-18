#pragma once

#if BLUE_COMPILER_IS_MSVC
#  include <Blue/System/Threading/Processor_Msvc.inl>
#elif BLUE_COMPILER_IS_CLANG || BLUE_COMPILER_IS_GCC
#  include <Blue/System/Threading/Processor_GccClang.inl>
#else
namespace Blue
{
BLUE_FORCE_INLINE void ProcessorPause( ) noexcept {}
} // namespace Blue
#endif

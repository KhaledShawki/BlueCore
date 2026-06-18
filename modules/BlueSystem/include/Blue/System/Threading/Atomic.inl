#pragma once

#if BLUE_COMPILER_IS_MSVC
#  include <Blue/System/Threading/Atomic_Msvc.inl>
#elif BLUE_COMPILER_IS_CLANG || BLUE_COMPILER_IS_GCC
#  include <Blue/System/Threading/Atomic_GccClang.inl>
#else
#  error BlueSystem atomics require MSVC, GCC, or Clang.
#endif

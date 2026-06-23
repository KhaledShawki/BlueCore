#pragma once

#include <Blue/Memory/Pool/MemoryPoolId.h>

namespace Blue
{
template< typename T >
struct MemoryPoolTrait
{
  static constexpr MemoryPoolId Pool = MemoryPoolId::System;
};

#define BLUE_USE_MEMORY_POOL( PoolName )                                                                               \
public:                                                                                                                \
  static constexpr ::Blue::MemoryPoolId BlueMemoryPoolIdValue = ::Blue::MemoryPoolId::PoolName;                        \
  static constexpr ::Blue::MemoryPoolId GetMemoryPoolId( ) noexcept                                                    \
  {                                                                                                                    \
    return BlueMemoryPoolIdValue;                                                                                      \
  }
} // namespace Blue

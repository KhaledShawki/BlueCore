#pragma once

#include <Blue/Memory/Pool/MemoryPoolTrait.h>
#include <Blue/System/Types.h>

namespace Blue
{
template< typename T >
concept HasBlueMemoryPool = requires {
  {
    T::GetMemoryPoolId( )
  };
};

template< typename T, Bool HasPool = HasBlueMemoryPool< T > >
struct MemoryPoolResolverImpl;

template< typename T >
struct MemoryPoolResolverImpl< T, true >
{
  static constexpr MemoryPoolId Pool = T::GetMemoryPoolId( );
};

template< typename T >
struct MemoryPoolResolverImpl< T, false >
{
  static constexpr MemoryPoolId Pool = MemoryPoolTrait< T >::Pool;
};

template< typename T >
struct MemoryPoolResolver
{
  static constexpr MemoryPoolId Pool = MemoryPoolResolverImpl< T >::Pool;
};
} // namespace Blue

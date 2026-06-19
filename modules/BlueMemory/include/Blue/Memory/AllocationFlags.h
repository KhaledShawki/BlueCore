#pragma once

#include <Blue/System/Types.h>

namespace Blue
{
enum AllocationFlags : Uint32
{
  AllocationFlag_None = 0,
  AllocationFlag_ZeroMemory = 1 << 0,
  AllocationFlag_NoTracking = 1 << 1,
  AllocationFlag_Temporary = 1 << 2,
};

constexpr Uint32 ToAllocationFlagMask( AllocationFlags flags ) noexcept
{
  return static_cast< Uint32 >( flags );
}

constexpr bool HasAllocationFlag( AllocationFlags flags, AllocationFlags flag ) noexcept
{
  return ( ToAllocationFlagMask( flags ) & ToAllocationFlagMask( flag ) ) != 0;
}
} // namespace Blue

#pragma once

#include <Blue/Memory/AllocationFailureReason.h>
#include <Blue/Memory/Allocator.h>
#include <Blue/System/Alignment.h>
#include <Blue/System/Types.h>

namespace Blue
{
struct AllocationValidationResult
{
  Bool Valid = false;
  AllocationFailureReason Reason = AllocationFailureReason::None;
};

constexpr AllocationValidationResult ValidateAllocationRequest( const AllocationRequest& request ) noexcept
{
  if ( request.ByteSize == 0 )
  {
    return { false, AllocationFailureReason::InvalidSize };
  }

  if ( !IsPowerOfTwo( request.Alignment ) )
  {
    return { false, AllocationFailureReason::InvalidAlignment };
  }

  if ( !IsValidMemoryPoolId( request.Pool ) )
  {
    return { false, AllocationFailureReason::InvalidPool };
  }

  return { true, AllocationFailureReason::None };
}
} // namespace Blue

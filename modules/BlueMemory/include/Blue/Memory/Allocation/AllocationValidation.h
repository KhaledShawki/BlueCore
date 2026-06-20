#pragma once

#include <Blue/Memory/AllocationFailureReason.h>
#include <Blue/Memory/Allocator.h>
#include <Blue/System/Alignment.h>
#include <Blue/System/Types.h>

#include <cstddef>

namespace Blue
{
struct AllocationValidationResult
{
  Bool Valid = false;
  AllocationFailureReason Reason = AllocationFailureReason::None;
};

constexpr Size MinimumAllocationAlignment( ) noexcept
{
  return alignof( std::max_align_t );
}

constexpr Size NormalizeAllocationAlignment( Size alignment ) noexcept
{
  const Size minimumAlignment = MinimumAllocationAlignment( );
  return alignment < minimumAlignment ? minimumAlignment : alignment;
}

constexpr Bool IsValidAllocationAlignment( Size alignment ) noexcept
{
  return alignment != 0 && IsPowerOfTwo( alignment );
}

constexpr AllocationRequest NormalizeAllocationRequest( AllocationRequest request ) noexcept
{
  request.Alignment = NormalizeAllocationAlignment( request.Alignment );
  return request;
}

constexpr AllocationValidationResult ValidateAllocationRequest( const AllocationRequest& request ) noexcept
{
  if ( request.ByteSize == 0 )
  {
    return { false, AllocationFailureReason::InvalidSize };
  }

  if ( !IsValidAllocationAlignment( request.Alignment ) )
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

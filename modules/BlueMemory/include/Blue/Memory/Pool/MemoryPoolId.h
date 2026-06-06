#pragma once

#include <Blue/System/Types.h>

namespace Blue
{
enum class MemoryPoolId : Uint16
{
#define BLUE_MEMORY_POOL( Id, Name, Budget, Allocator, Metrics, Oom ) Id,
#include <Blue/Memory/Pool/MemoryPools.def>
#undef BLUE_MEMORY_POOL
	Count
};

constexpr Size MemoryPoolCount = static_cast< Size >( MemoryPoolId::Count );

constexpr Size ToMemoryPoolIndex( MemoryPoolId pool ) noexcept
{
	return static_cast< Size >( pool );
}

constexpr Bool IsValidMemoryPoolId( MemoryPoolId pool ) noexcept
{
	return ToMemoryPoolIndex( pool ) < MemoryPoolCount;
}
} // namespace Blue

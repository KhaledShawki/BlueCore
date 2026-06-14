#pragma once

#include <Blue/Memory/Api.h>
#include <Blue/System/Types.h>

namespace Blue
{
enum class AllocatorKind : Uint8
{
	Default,
	Heap,
	Linear,
	Stack,
	FixedPool,
	Slot,
	Tlsf,
	BigBlock,
	Frame,
	Count
};

BLUE_MEMORY_API const Char* GetAllocatorKindName( AllocatorKind kind ) noexcept;
} // namespace Blue

#pragma once

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

const Char* GetAllocatorKindName( AllocatorKind kind ) noexcept;
} // namespace Blue

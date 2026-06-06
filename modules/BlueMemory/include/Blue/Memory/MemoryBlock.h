#pragma once

#include <Blue/Memory/AllocationTag.h>
#include <Blue/System/Types.h>

namespace Blue
{
enum MemoryBlockFlags : Uint32
{
	MemoryBlockFlag_None = 0,
	MemoryBlockFlag_Owned = 1 << 0,
	MemoryBlockFlag_ReadOnly = 1 << 1,
	MemoryBlockFlag_Executable = 1 << 2,
	MemoryBlockFlag_Tracked = 1 << 3,
	MemoryBlockFlag_Serialized = 1 << 4,
};

struct MemoryBlock
{
	void* Base;
	Size ByteSize;
	Size Alignment;
	AllocationTag Tag;
	Uint32 AllocatorId;
	Uint32 Flags;
};

inline bool IsValid( const MemoryBlock& block )
{
	return block.Base != nullptr && block.ByteSize > 0;
}

inline void* GetEnd( const MemoryBlock& block )
{
	return static_cast< Byte* >( block.Base ) + block.ByteSize;
}

inline bool Contains( const MemoryBlock& block, const void* pointer )
{
	const Byte* begin = static_cast< const Byte* >( block.Base );
	const Byte* end = begin + block.ByteSize;
	const Byte* value = static_cast< const Byte* >( pointer );
	return value >= begin && value < end;
}
} // namespace Blue

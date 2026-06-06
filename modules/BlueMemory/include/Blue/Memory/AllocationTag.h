#pragma once

#include <Blue/System/Types.h>

namespace Blue
{
enum class AllocationTag : Uint32
{
	Unknown,
	System,
	Memory,
	Object,
	Container,
	String,
	JobSystem,
	Test,
	ThirdParty,
	ResourceBuffer,
	User,
};
} // namespace Blue

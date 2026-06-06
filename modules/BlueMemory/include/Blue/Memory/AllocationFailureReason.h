#pragma once

#include <Blue/System/Types.h>

namespace Blue
{
enum class AllocationFailureReason : Uint8
{
	None,
	OutOfMemory,
	InvalidSize,
	InvalidAlignment,
	InvalidPool,
	PoolBudgetExceeded,
	SystemNotInitialized,
	BackendFailure
};
} // namespace Blue

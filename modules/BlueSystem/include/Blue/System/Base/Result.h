#pragma once

#include <Blue/System/Api.h>
#include <Blue/System/Types.h>

namespace Blue
{
enum class ResultCode : Uint32
{
	Success,
	InvalidArgument,
	OutOfMemory,
	NotInitialized,
	AlreadyInitialized,
	PlatformError,
	Unsupported,
	CapacityExceeded,
	Timeout,
	Busy,
	UnknownFailure,
};

struct Result
{
	ResultCode Code;

	constexpr Bool Succeeded( ) const;
	constexpr Bool Failed( ) const;
};

constexpr Result Success( );
constexpr Result Failure( ResultCode code );

BLUE_SYSTEM_API const Char* GetResultCodeName( ResultCode code );
} // namespace Blue

#include <Blue/System/Base/Result.inl>

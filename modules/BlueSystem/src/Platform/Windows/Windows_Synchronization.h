#pragma once

#include <Blue/System/Time.h>
#include <Blue/System/Types.h>

#include <Blue/System/Platform/WindowsLean.h>

namespace Blue::Windows
{
inline DWORD ToTimeoutMilliseconds( TimeDuration timeout ) noexcept
{
	if ( timeout.Nanoseconds == 0 )
	{
		return 0;
	}

	constexpr Uint64 MaxWaitMilliseconds = 0xFFFFFFFEull;
	constexpr Uint64 RoundingOffset = NanosecondsPerMillisecond - 1;
	const Uint64 roundedNanoseconds = Internal::SaturatingAddUint64( timeout.Nanoseconds, RoundingOffset );
	const Uint64 milliseconds = roundedNanoseconds / NanosecondsPerMillisecond;

	return milliseconds > MaxWaitMilliseconds ? static_cast< DWORD >( MaxWaitMilliseconds )
	                                          : static_cast< DWORD >( milliseconds );
}
} // namespace Blue::Windows

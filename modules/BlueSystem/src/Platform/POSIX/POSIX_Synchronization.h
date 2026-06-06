#pragma once

#include <Blue/System/Time.h>
#include <Blue/System/Types.h>

#include <errno.h>
#include <pthread.h>
#include <time.h>

namespace Blue::POSIX
{
#if defined( CLOCK_MONOTONIC ) && !defined( __APPLE__ )
constexpr clockid_t ConditionVariableClock = CLOCK_MONOTONIC;
constexpr Bool SupportsConditionVariableClockSelection = true;
#else
constexpr clockid_t ConditionVariableClock = CLOCK_REALTIME;
constexpr Bool SupportsConditionVariableClockSelection = false;
#endif

Bool InitializeConditionVariable( pthread_cond_t& conditionVariable ) noexcept;
Bool MakeAbsoluteTimeoutFromNow( TimeDuration timeout, timespec& outDeadline ) noexcept;
Bool IsTimeoutResult( int result ) noexcept;
} // namespace Blue::POSIX

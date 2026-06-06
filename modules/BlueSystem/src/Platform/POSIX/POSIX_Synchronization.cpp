#include "POSIX_Synchronization.h"

#include <limits>

namespace Blue::POSIX
{
namespace
{
constexpr Uint64 MaxUint64 = ~static_cast< Uint64 >( 0 );

Uint64 SaturatingAdd( Uint64 left, Uint64 right ) noexcept
{
	if ( MaxUint64 - left < right )
	{
		return MaxUint64;
	}

	return left + right;
}
} // namespace

Bool InitializeConditionVariable( pthread_cond_t& conditionVariable ) noexcept
{
	pthread_condattr_t attributes;
	if ( pthread_condattr_init( &attributes ) != 0 )
	{
		return false;
	}

#if defined( CLOCK_MONOTONIC ) && !defined( __APPLE__ )
	if ( pthread_condattr_setclock( &attributes, ConditionVariableClock ) != 0 )
	{
		pthread_condattr_destroy( &attributes );
		return false;
	}
#endif

	const int result = pthread_cond_init( &conditionVariable, &attributes );
	pthread_condattr_destroy( &attributes );
	return result == 0;
}

Bool MakeAbsoluteTimeoutFromNow( TimeDuration timeout, timespec& outDeadline ) noexcept
{
	timespec now{ };
	if ( clock_gettime( ConditionVariableClock, &now ) != 0 )
	{
		return false;
	}

	const Uint64 timeoutSeconds = timeout.Nanoseconds / NanosecondsPerSecond;
	const Uint64 timeoutNanoseconds = timeout.Nanoseconds % NanosecondsPerSecond;

	const Uint64 nowSeconds = now.tv_sec < 0 ? 0 : static_cast< Uint64 >( now.tv_sec );
	const Uint64 secondsWithTimeout = SaturatingAdd( nowSeconds, timeoutSeconds );
	const Uint64 nanosecondsWithTimeout = static_cast< Uint64 >( now.tv_nsec ) + timeoutNanoseconds;
	const Uint64 carrySeconds = nanosecondsWithTimeout / NanosecondsPerSecond;
	const Uint64 finalSeconds = SaturatingAdd( secondsWithTimeout, carrySeconds );

	const Uint64 maxTimeT = static_cast< Uint64 >( std::numeric_limits< time_t >::max( ) );
	outDeadline.tv_sec =
	    finalSeconds > maxTimeT ? std::numeric_limits< time_t >::max( ) : static_cast< time_t >( finalSeconds );
	outDeadline.tv_nsec = static_cast< long >( nanosecondsWithTimeout % NanosecondsPerSecond );
	return true;
}

Bool IsTimeoutResult( int result ) noexcept
{
	return result == ETIMEDOUT;
}
} // namespace Blue::POSIX

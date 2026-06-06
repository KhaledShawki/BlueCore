#pragma once

#include <Blue/System/Api.h>
#include <Blue/System/Compiler.h>
#include <Blue/System/NonCopyable.h>
#include <Blue/System/Time.h>
#include <Blue/System/Types.h>

namespace Blue
{
enum class EventResetMode : Uint8
{
	Auto,
	Manual,
};

struct NativeEventHandle final
{
	alignas( 8 ) Byte Storage[ 192 ] = { };
};

struct EventCreateDesc final
{
	EventResetMode ResetMode = EventResetMode::Manual;
	Bool InitiallySignaled = false;
};

struct Event final : private NonCopyable
{
	NativeEventHandle NativeHandle = { };
	Bool Initialized = false;

	void Signal( ) noexcept;
	void Reset( ) noexcept;
	void Wait( ) noexcept;
	Bool TryWait( ) noexcept;
	Bool WaitFor( TimeDuration timeout ) noexcept;
};

BLUE_SYSTEM_API Bool InitializeEvent( Event& event, const EventCreateDesc& desc = { } ) noexcept;
BLUE_SYSTEM_API void ShutdownEvent( Event& event ) noexcept;

BLUE_SYSTEM_API void SignalEvent( Event& event ) noexcept;
BLUE_SYSTEM_API void ResetEvent( Event& event ) noexcept;
BLUE_SYSTEM_API void WaitEvent( Event& event ) noexcept;
BLUE_SYSTEM_API Bool TryWaitEvent( Event& event ) noexcept;
BLUE_SYSTEM_API Bool WaitEventFor( Event& event, TimeDuration timeout ) noexcept;

Bool IsEventInitialized( const Event& event ) noexcept;

class OwnedEvent final : private NonCopyable
{
public:
	explicit OwnedEvent( const EventCreateDesc& desc = { } ) noexcept;
	~OwnedEvent( ) noexcept;

	Bool IsValid( ) const noexcept;
	Event& Get( ) noexcept;
	const Event& Get( ) const noexcept;

	void Signal( ) noexcept;
	void Reset( ) noexcept;
	void Wait( ) noexcept;
	Bool TryWait( ) noexcept;
	Bool WaitFor( TimeDuration timeout ) noexcept;

private:
	Event m_Event = { };
};
} // namespace Blue

#include <Blue/System/Threading/Event.inl>

#pragma once

namespace Blue
{
BLUE_FORCE_INLINE Bool IsEventInitialized( const Event& event ) noexcept
{
	return event.Initialized;
}

BLUE_FORCE_INLINE void Event::Signal( ) noexcept
{
	SignalEvent( *this );
}

BLUE_FORCE_INLINE void Event::Reset( ) noexcept
{
	ResetEvent( *this );
}

BLUE_FORCE_INLINE void Event::Wait( ) noexcept
{
	WaitEvent( *this );
}

BLUE_FORCE_INLINE Bool Event::TryWait( ) noexcept
{
	return TryWaitEvent( *this );
}

BLUE_FORCE_INLINE Bool Event::WaitFor( TimeDuration timeout ) noexcept
{
	return WaitEventFor( *this, timeout );
}

BLUE_FORCE_INLINE OwnedEvent::OwnedEvent( const EventCreateDesc& desc ) noexcept
{
	InitializeEvent( m_Event, desc );
}

BLUE_FORCE_INLINE OwnedEvent::~OwnedEvent( ) noexcept
{
	ShutdownEvent( m_Event );
}

BLUE_FORCE_INLINE Bool OwnedEvent::IsValid( ) const noexcept
{
	return IsEventInitialized( m_Event );
}

BLUE_FORCE_INLINE Event& OwnedEvent::Get( ) noexcept
{
	return m_Event;
}

BLUE_FORCE_INLINE const Event& OwnedEvent::Get( ) const noexcept
{
	return m_Event;
}

BLUE_FORCE_INLINE void OwnedEvent::Signal( ) noexcept
{
	SignalEvent( m_Event );
}

BLUE_FORCE_INLINE void OwnedEvent::Reset( ) noexcept
{
	ResetEvent( m_Event );
}

BLUE_FORCE_INLINE void OwnedEvent::Wait( ) noexcept
{
	WaitEvent( m_Event );
}

BLUE_FORCE_INLINE Bool OwnedEvent::TryWait( ) noexcept
{
	return TryWaitEvent( m_Event );
}

BLUE_FORCE_INLINE Bool OwnedEvent::WaitFor( TimeDuration timeout ) noexcept
{
	return WaitEventFor( m_Event, timeout );
}
} // namespace Blue

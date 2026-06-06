#pragma once

namespace Blue
{
BLUE_FORCE_INLINE OwnedThread::OwnedThread( const ThreadCreateInfo& createInfo ) noexcept
{
	Create( createInfo );
}

BLUE_FORCE_INLINE OwnedThread::~OwnedThread( ) noexcept
{
	Detach( );
}

BLUE_FORCE_INLINE Bool OwnedThread::Create( const ThreadCreateInfo& createInfo ) noexcept
{
	if ( IsThreadJoinable( m_Thread ) )
	{
		return false;
	}

	return CreateThread( m_Thread, createInfo );
}

BLUE_FORCE_INLINE Bool OwnedThread::Join( Uint32* outExitCode ) noexcept
{
	return JoinThread( m_Thread, outExitCode );
}

BLUE_FORCE_INLINE Bool OwnedThread::Detach( ) noexcept
{
	if ( !IsThreadJoinable( m_Thread ) )
	{
		return true;
	}

	return DetachThread( m_Thread );
}

BLUE_FORCE_INLINE Bool OwnedThread::IsJoinable( ) const noexcept
{
	return IsThreadJoinable( m_Thread );
}

BLUE_FORCE_INLINE ThreadId OwnedThread::GetId( ) const noexcept
{
	return m_Thread.Id;
}

BLUE_FORCE_INLINE Bool OwnedThread::SetPriority( ThreadPriority priority ) noexcept
{
	return SetThreadPriority( m_Thread, priority );
}

BLUE_FORCE_INLINE Bool OwnedThread::SetAffinity( CpuAffinity affinity ) noexcept
{
	return SetThreadAffinity( m_Thread, affinity );
}

BLUE_FORCE_INLINE Thread& OwnedThread::Get( ) noexcept
{
	return m_Thread;
}

BLUE_FORCE_INLINE const Thread& OwnedThread::Get( ) const noexcept
{
	return m_Thread;
}
} // namespace Blue

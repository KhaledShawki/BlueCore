#pragma once

namespace Blue
{
BLUE_FORCE_INLINE Bool IsMutexInitialized( const Mutex& mutex ) noexcept
{
	return mutex.Initialized;
}

BLUE_FORCE_INLINE void Mutex::Acquire( ) noexcept
{
	AcquireMutex( *this );
}

BLUE_FORCE_INLINE Bool Mutex::TryAcquire( ) noexcept
{
	return TryAcquireMutex( *this );
}

BLUE_FORCE_INLINE void Mutex::Release( ) noexcept
{
	ReleaseMutex( *this );
}

BLUE_FORCE_INLINE void LockMutex( Mutex& mutex ) noexcept
{
	AcquireMutex( mutex );
}

BLUE_FORCE_INLINE Bool TryLockMutex( Mutex& mutex ) noexcept
{
	return TryAcquireMutex( mutex );
}

BLUE_FORCE_INLINE void UnlockMutex( Mutex& mutex ) noexcept
{
	ReleaseMutex( mutex );
}

BLUE_FORCE_INLINE ScopedMutexLock::ScopedMutexLock( Mutex& mutex ) noexcept
    : m_Mutex( &mutex )
{
	m_Mutex->Acquire( );
}

BLUE_FORCE_INLINE ScopedMutexLock::~ScopedMutexLock( ) noexcept
{
	m_Mutex->Release( );
}

BLUE_FORCE_INLINE OwnedMutex::OwnedMutex( ) noexcept
{
	InitializeMutex( m_Mutex );
}

BLUE_FORCE_INLINE OwnedMutex::~OwnedMutex( ) noexcept
{
	ShutdownMutex( m_Mutex );
}

BLUE_FORCE_INLINE Bool OwnedMutex::IsValid( ) const noexcept
{
	return IsMutexInitialized( m_Mutex );
}

BLUE_FORCE_INLINE Mutex& OwnedMutex::Get( ) noexcept
{
	return m_Mutex;
}

BLUE_FORCE_INLINE const Mutex& OwnedMutex::Get( ) const noexcept
{
	return m_Mutex;
}

BLUE_FORCE_INLINE void OwnedMutex::Acquire( ) noexcept
{
	AcquireMutex( m_Mutex );
}

BLUE_FORCE_INLINE Bool OwnedMutex::TryAcquire( ) noexcept
{
	return TryAcquireMutex( m_Mutex );
}

BLUE_FORCE_INLINE void OwnedMutex::Release( ) noexcept
{
	ReleaseMutex( m_Mutex );
}
} // namespace Blue

#pragma once

namespace Blue
{
BLUE_FORCE_INLINE Bool IsSemaphoreInitialized( const Semaphore& semaphore ) noexcept
{
  return semaphore.Initialized;
}

BLUE_FORCE_INLINE void Semaphore::Acquire( ) noexcept
{
  AcquireSemaphore( *this );
}

BLUE_FORCE_INLINE Bool Semaphore::TryAcquire( ) noexcept
{
  return TryAcquireSemaphore( *this );
}

BLUE_FORCE_INLINE Bool Semaphore::AcquireFor( TimeDuration timeout ) noexcept
{
  return AcquireSemaphoreFor( *this, timeout );
}

BLUE_FORCE_INLINE Bool Semaphore::Release( Uint32 count ) noexcept
{
  return ReleaseSemaphore( *this, count );
}

BLUE_FORCE_INLINE OwnedSemaphore::OwnedSemaphore( const SemaphoreCreateDesc& desc ) noexcept
{
  InitializeSemaphore( m_Semaphore, desc );
}

BLUE_FORCE_INLINE OwnedSemaphore::~OwnedSemaphore( ) noexcept
{
  ShutdownSemaphore( m_Semaphore );
}

BLUE_FORCE_INLINE Bool OwnedSemaphore::IsValid( ) const noexcept
{
  return IsSemaphoreInitialized( m_Semaphore );
}

BLUE_FORCE_INLINE Semaphore& OwnedSemaphore::Get( ) noexcept
{
  return m_Semaphore;
}

BLUE_FORCE_INLINE const Semaphore& OwnedSemaphore::Get( ) const noexcept
{
  return m_Semaphore;
}

BLUE_FORCE_INLINE void OwnedSemaphore::Acquire( ) noexcept
{
  AcquireSemaphore( m_Semaphore );
}

BLUE_FORCE_INLINE Bool OwnedSemaphore::TryAcquire( ) noexcept
{
  return TryAcquireSemaphore( m_Semaphore );
}

BLUE_FORCE_INLINE Bool OwnedSemaphore::AcquireFor( TimeDuration timeout ) noexcept
{
  return AcquireSemaphoreFor( m_Semaphore, timeout );
}

BLUE_FORCE_INLINE Bool OwnedSemaphore::Release( Uint32 count ) noexcept
{
  return ReleaseSemaphore( m_Semaphore, count );
}
} // namespace Blue

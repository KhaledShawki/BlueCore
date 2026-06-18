#pragma once

namespace Blue
{
BLUE_FORCE_INLINE SpinLock::SpinLock( ) noexcept
    : m_State( 0 )
{}

BLUE_FORCE_INLINE Bool SpinLock::TryAcquire( ) noexcept
{
  Uint32 expected = 0;
  return m_State.CompareExchange( expected, 1, MemoryOrder::Acquire, MemoryOrder::Relaxed );
}

BLUE_FORCE_INLINE void SpinLock::Acquire( ) noexcept
{
  constexpr Uint32 PauseSpinCount = 64;
  constexpr Uint32 YieldSpinCount = 128;

  Uint32 spinCount = 0;

  while ( !TryAcquire( ) )
  {
    if ( spinCount < PauseSpinCount )
    {
      ProcessorPause( );
    }
    else if ( spinCount < YieldSpinCount )
    {
      ProcessorPause( );
      ProcessorPause( );
    }
    else
    {
      YieldThread( );
    }

    ++spinCount;
  }
}

BLUE_FORCE_INLINE void SpinLock::Release( ) noexcept
{
  m_State.Store( 0, MemoryOrder::Release );
}

BLUE_FORCE_INLINE void SpinLock::Lock( ) noexcept
{
  Acquire( );
}

BLUE_FORCE_INLINE Bool SpinLock::TryLock( ) noexcept
{
  return TryAcquire( );
}

BLUE_FORCE_INLINE void SpinLock::Unlock( ) noexcept
{
  Release( );
}

BLUE_FORCE_INLINE ScopedSpinLock::ScopedSpinLock( SpinLock& lock ) noexcept
    : m_Lock( &lock )
{
  m_Lock->Acquire( );
}

BLUE_FORCE_INLINE ScopedSpinLock::~ScopedSpinLock( ) noexcept
{
  m_Lock->Release( );
}
} // namespace Blue

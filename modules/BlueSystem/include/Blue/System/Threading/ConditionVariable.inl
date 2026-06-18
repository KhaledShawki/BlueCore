#pragma once

namespace Blue
{
BLUE_FORCE_INLINE Bool IsConditionVariableInitialized( const ConditionVariable& conditionVariable ) noexcept
{
  return conditionVariable.Initialized;
}

BLUE_FORCE_INLINE void ConditionVariable::Wait( Mutex& mutex ) noexcept
{
  WaitConditionVariable( *this, mutex );
}

BLUE_FORCE_INLINE Bool ConditionVariable::WaitFor( Mutex& mutex, TimeDuration timeout ) noexcept
{
  return WaitConditionVariableFor( *this, mutex, timeout );
}

BLUE_FORCE_INLINE void ConditionVariable::NotifyOne( ) noexcept
{
  NotifyOneConditionVariable( *this );
}

BLUE_FORCE_INLINE void ConditionVariable::NotifyAll( ) noexcept
{
  NotifyAllConditionVariable( *this );
}

BLUE_FORCE_INLINE OwnedConditionVariable::OwnedConditionVariable( ) noexcept
{
  InitializeConditionVariable( m_ConditionVariable );
}

BLUE_FORCE_INLINE OwnedConditionVariable::~OwnedConditionVariable( ) noexcept
{
  ShutdownConditionVariable( m_ConditionVariable );
}

BLUE_FORCE_INLINE Bool OwnedConditionVariable::IsValid( ) const noexcept
{
  return IsConditionVariableInitialized( m_ConditionVariable );
}

BLUE_FORCE_INLINE ConditionVariable& OwnedConditionVariable::Get( ) noexcept
{
  return m_ConditionVariable;
}

BLUE_FORCE_INLINE const ConditionVariable& OwnedConditionVariable::Get( ) const noexcept
{
  return m_ConditionVariable;
}

BLUE_FORCE_INLINE void OwnedConditionVariable::Wait( Mutex& mutex ) noexcept
{
  WaitConditionVariable( m_ConditionVariable, mutex );
}

BLUE_FORCE_INLINE Bool OwnedConditionVariable::WaitFor( Mutex& mutex, TimeDuration timeout ) noexcept
{
  return WaitConditionVariableFor( m_ConditionVariable, mutex, timeout );
}

BLUE_FORCE_INLINE void OwnedConditionVariable::NotifyOne( ) noexcept
{
  NotifyOneConditionVariable( m_ConditionVariable );
}

BLUE_FORCE_INLINE void OwnedConditionVariable::NotifyAll( ) noexcept
{
  NotifyAllConditionVariable( m_ConditionVariable );
}
} // namespace Blue

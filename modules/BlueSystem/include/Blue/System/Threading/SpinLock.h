#pragma once

#include <Blue/System/Threading/Atomic.h>
#include <Blue/System/Threading/Processor.h>

namespace Blue
{
class SpinLock final
{
public:
  SpinLock( ) noexcept;

  SpinLock( const SpinLock& ) = delete;
  SpinLock& operator=( const SpinLock& ) = delete;

  void Acquire( ) noexcept;
  Bool TryAcquire( ) noexcept;
  void Release( ) noexcept;

  void Lock( ) noexcept;
  Bool TryLock( ) noexcept;
  void Unlock( ) noexcept;

private:
  AtomicUint32 m_State;
};

class ScopedSpinLock final
{
public:
  explicit ScopedSpinLock( SpinLock& lock ) noexcept;
  ~ScopedSpinLock( ) noexcept;

  ScopedSpinLock( const ScopedSpinLock& ) = delete;
  ScopedSpinLock& operator=( const ScopedSpinLock& ) = delete;

private:
  SpinLock* m_Lock;
};
} // namespace Blue

#include <Blue/System/Threading/SpinLock.inl>

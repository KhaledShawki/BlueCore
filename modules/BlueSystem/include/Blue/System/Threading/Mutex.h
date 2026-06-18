#pragma once

#include <Blue/System/Api.h>
#include <Blue/System/Compiler.h>
#include <Blue/System/NonCopyable.h>
#include <Blue/System/Types.h>

namespace Blue
{
struct NativeMutexHandle final
{
  alignas( 8 ) Byte Storage[ 64 ] = { };
};

struct Mutex final : private NonCopyable
{
  NativeMutexHandle NativeHandle = { };
  Bool Initialized = false;

  void Acquire( ) noexcept;
  Bool TryAcquire( ) noexcept;
  void Release( ) noexcept;
};

BLUE_SYSTEM_API Bool InitializeMutex( Mutex& mutex ) noexcept;
BLUE_SYSTEM_API void ShutdownMutex( Mutex& mutex ) noexcept;

BLUE_SYSTEM_API void AcquireMutex( Mutex& mutex ) noexcept;
BLUE_SYSTEM_API Bool TryAcquireMutex( Mutex& mutex ) noexcept;
BLUE_SYSTEM_API void ReleaseMutex( Mutex& mutex ) noexcept;

void LockMutex( Mutex& mutex ) noexcept;
Bool TryLockMutex( Mutex& mutex ) noexcept;
void UnlockMutex( Mutex& mutex ) noexcept;

Bool IsMutexInitialized( const Mutex& mutex ) noexcept;

class ScopedMutexLock final : private NonCopyable
{
  public:
  explicit ScopedMutexLock( Mutex& mutex ) noexcept;
  ~ScopedMutexLock( ) noexcept;

  private:
  Mutex* m_Mutex;
};

class OwnedMutex final : private NonCopyable
{
  public:
  OwnedMutex( ) noexcept;
  ~OwnedMutex( ) noexcept;

  Bool IsValid( ) const noexcept;
  Mutex& Get( ) noexcept;
  const Mutex& Get( ) const noexcept;

  void Acquire( ) noexcept;
  Bool TryAcquire( ) noexcept;
  void Release( ) noexcept;

  private:
  Mutex m_Mutex = { };
};
} // namespace Blue

#include <Blue/System/Threading/Mutex.inl>

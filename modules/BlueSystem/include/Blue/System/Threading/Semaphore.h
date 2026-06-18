#pragma once

#include <Blue/System/Api.h>
#include <Blue/System/Compiler.h>
#include <Blue/System/NonCopyable.h>
#include <Blue/System/Time.h>
#include <Blue/System/Types.h>

namespace Blue
{
struct NativeSemaphoreHandle final
{
  alignas( 8 ) Byte Storage[ 192 ] = { };
};

struct SemaphoreCreateDesc final
{
  Uint32 InitialCount = 0;
  Uint32 MaximumCount = 0x7FFFFFFFu;
};

struct Semaphore final : private NonCopyable
{
  NativeSemaphoreHandle NativeHandle = { };
  Bool Initialized = false;

  void Acquire( ) noexcept;
  Bool TryAcquire( ) noexcept;
  Bool AcquireFor( TimeDuration timeout ) noexcept;
  Bool Release( Uint32 count = 1 ) noexcept;
};

BLUE_SYSTEM_API Bool InitializeSemaphore( Semaphore& semaphore, const SemaphoreCreateDesc& desc = { } ) noexcept;
BLUE_SYSTEM_API void ShutdownSemaphore( Semaphore& semaphore ) noexcept;

BLUE_SYSTEM_API void AcquireSemaphore( Semaphore& semaphore ) noexcept;
BLUE_SYSTEM_API Bool TryAcquireSemaphore( Semaphore& semaphore ) noexcept;
BLUE_SYSTEM_API Bool AcquireSemaphoreFor( Semaphore& semaphore, TimeDuration timeout ) noexcept;
BLUE_SYSTEM_API Bool ReleaseSemaphore( Semaphore& semaphore, Uint32 count = 1 ) noexcept;

Bool IsSemaphoreInitialized( const Semaphore& semaphore ) noexcept;

class OwnedSemaphore final : private NonCopyable
{
  public:
  explicit OwnedSemaphore( const SemaphoreCreateDesc& desc = { } ) noexcept;
  ~OwnedSemaphore( ) noexcept;

  Bool IsValid( ) const noexcept;
  Semaphore& Get( ) noexcept;
  const Semaphore& Get( ) const noexcept;

  void Acquire( ) noexcept;
  Bool TryAcquire( ) noexcept;
  Bool AcquireFor( TimeDuration timeout ) noexcept;
  Bool Release( Uint32 count = 1 ) noexcept;

  private:
  Semaphore m_Semaphore = { };
};
} // namespace Blue

#include <Blue/System/Threading/Semaphore.inl>

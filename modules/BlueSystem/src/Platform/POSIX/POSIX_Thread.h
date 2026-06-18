#pragma once

#include <Blue/System/Threading/Thread.h>

#include <pthread.h>

namespace Blue::Internal
{
inline constexpr Size POSIXThreadNameCapacity = 64;

struct POSIXThreadStartContext final
{
  ThreadEntryFn Entry = nullptr;
  void* UserData = nullptr;
  Char Name[ POSIXThreadNameCapacity ] = { };
  Bool HasName = false;
  ThreadPriority Priority = ThreadPriority::Normal;
  CpuAffinity Affinity = CpuAffinity::Any( );
};

void ClearNativeThreadHandle( NativeThreadHandle& handle ) noexcept;
void StoreNativeThreadHandle( NativeThreadHandle& handle, pthread_t nativeHandle ) noexcept;
pthread_t LoadNativeThreadHandle( const NativeThreadHandle& handle ) noexcept;
void ResetThread( Thread& thread ) noexcept;

ThreadId GetThreadIdFromPthread( pthread_t thread ) noexcept;
void CopyThreadName( Char* destination, Size destinationCapacity, const Char* source ) noexcept;
} // namespace Blue::Internal

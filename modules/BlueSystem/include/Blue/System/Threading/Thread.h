#pragma once

#include <Blue/System/Api.h>
#include <Blue/System/NonCopyable.h>
#include <Blue/System/Threading/Processor.h>
#include <Blue/System/Threading/ThreadTypes.h>

namespace Blue
{
BLUE_SYSTEM_API Bool CreateThread( Thread& outThread, const ThreadCreateInfo& createInfo ) noexcept;
BLUE_SYSTEM_API Bool JoinThread( Thread& thread, Uint32* outExitCode = nullptr ) noexcept;
BLUE_SYSTEM_API Bool DetachThread( Thread& thread ) noexcept;

BLUE_SYSTEM_API Bool IsThreadJoinable( const Thread& thread ) noexcept;
BLUE_SYSTEM_API ThreadId GetCurrentThreadId( ) noexcept;

BLUE_SYSTEM_API void SetCurrentThreadName( const Char* name ) noexcept;

BLUE_SYSTEM_API Bool SetThreadPriority( Thread& thread, ThreadPriority priority ) noexcept;
BLUE_SYSTEM_API Bool SetCurrentThreadPriority( ThreadPriority priority ) noexcept;

BLUE_SYSTEM_API Bool SetThreadAffinity( Thread& thread, CpuAffinity affinity ) noexcept;
BLUE_SYSTEM_API Bool SetCurrentThreadAffinity( CpuAffinity affinity ) noexcept;

class OwnedThread final : private NonCopyable
{
public:
  OwnedThread( ) noexcept = default;
  explicit OwnedThread( const ThreadCreateInfo& createInfo ) noexcept;
  ~OwnedThread( ) noexcept;

  Bool Create( const ThreadCreateInfo& createInfo ) noexcept;
  Bool Join( Uint32* outExitCode = nullptr ) noexcept;
  Bool Detach( ) noexcept;

  Bool IsJoinable( ) const noexcept;
  ThreadId GetId( ) const noexcept;

  Bool SetPriority( ThreadPriority priority ) noexcept;
  Bool SetAffinity( CpuAffinity affinity ) noexcept;

  Thread& Get( ) noexcept;
  const Thread& Get( ) const noexcept;

private:
  Thread m_Thread = { };
};
} // namespace Blue

#include <Blue/System/Threading/Thread.inl>

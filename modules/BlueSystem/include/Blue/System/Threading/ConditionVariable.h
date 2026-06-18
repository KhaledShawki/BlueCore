#pragma once

#include <Blue/System/Api.h>
#include <Blue/System/Compiler.h>
#include <Blue/System/Mutex.h>
#include <Blue/System/NonCopyable.h>
#include <Blue/System/Time.h>
#include <Blue/System/Types.h>

namespace Blue
{
struct NativeConditionVariableHandle final
{
  alignas( 8 ) Byte Storage[ 128 ] = { };
};

struct ConditionVariable final : private NonCopyable
{
  NativeConditionVariableHandle NativeHandle = { };
  Bool Initialized = false;

  void Wait( Mutex& mutex ) noexcept;
  Bool WaitFor( Mutex& mutex, TimeDuration timeout ) noexcept;
  void NotifyOne( ) noexcept;
  void NotifyAll( ) noexcept;
};

BLUE_SYSTEM_API Bool InitializeConditionVariable( ConditionVariable& conditionVariable ) noexcept;
BLUE_SYSTEM_API void ShutdownConditionVariable( ConditionVariable& conditionVariable ) noexcept;

BLUE_SYSTEM_API void WaitConditionVariable( ConditionVariable& conditionVariable, Mutex& mutex ) noexcept;
BLUE_SYSTEM_API Bool WaitConditionVariableFor( ConditionVariable& conditionVariable,
                                               Mutex& mutex,
                                               TimeDuration timeout ) noexcept;
BLUE_SYSTEM_API void NotifyOneConditionVariable( ConditionVariable& conditionVariable ) noexcept;
BLUE_SYSTEM_API void NotifyAllConditionVariable( ConditionVariable& conditionVariable ) noexcept;

Bool IsConditionVariableInitialized( const ConditionVariable& conditionVariable ) noexcept;

class OwnedConditionVariable final : private NonCopyable
{
  public:
  OwnedConditionVariable( ) noexcept;
  ~OwnedConditionVariable( ) noexcept;

  Bool IsValid( ) const noexcept;
  ConditionVariable& Get( ) noexcept;
  const ConditionVariable& Get( ) const noexcept;

  void Wait( Mutex& mutex ) noexcept;
  Bool WaitFor( Mutex& mutex, TimeDuration timeout ) noexcept;
  void NotifyOne( ) noexcept;
  void NotifyAll( ) noexcept;

  private:
  ConditionVariable m_ConditionVariable = { };
};
} // namespace Blue

#include <Blue/System/Threading/ConditionVariable.inl>

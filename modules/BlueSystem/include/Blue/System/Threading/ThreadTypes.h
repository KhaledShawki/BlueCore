#pragma once

#include <Blue/System/Compiler.h>
#include <Blue/System/Types.h>

namespace Blue
{
using ThreadId = Uint64;
using ThreadEntryFn = Uint32 ( * )( void* userData );

enum class ThreadPriority : Uint8
{
  Low,
  Normal,
  High,
  Critical,
};

struct CpuAffinity final
{
  Uint64 ProcessorMask = 0;

  constexpr CpuAffinity( ) noexcept = default;

  constexpr explicit CpuAffinity( Uint64 processorMask ) noexcept
      : ProcessorMask( processorMask )
  {}

  static constexpr CpuAffinity Any( ) noexcept { return CpuAffinity{ }; }

  static constexpr CpuAffinity FromMask( Uint64 processorMask ) noexcept { return CpuAffinity( processorMask ); }

  static constexpr CpuAffinity FromProcessorIndex( Uint32 processorIndex ) noexcept
  {
    return processorIndex < 64 ? CpuAffinity( Uint64{ 1 } << processorIndex ) : CpuAffinity{ };
  }

  constexpr Bool IsEnabled( ) const noexcept { return ProcessorMask != 0; }

  constexpr Bool ContainsProcessor( Uint32 processorIndex ) const noexcept
  {
    return processorIndex < 64 && ( ProcessorMask & ( Uint64{ 1 } << processorIndex ) ) != 0;
  }
};

struct NativeThreadHandle final
{
  alignas( 8 ) Byte Storage[ 16 ] = { };
};

struct ThreadCreateInfo final
{
  const Char* Name = nullptr;
  ThreadEntryFn Entry = nullptr;
  void* UserData = nullptr;
  ThreadPriority Priority = ThreadPriority::Normal;
  CpuAffinity Affinity = CpuAffinity::Any( );
  Size StackSize = 0;
};

using ThreadCreateDesc = ThreadCreateInfo;

struct Thread final
{
  NativeThreadHandle NativeHandle = { };
  ThreadId Id = 0;
  Bool Joinable = false;
};
} // namespace Blue

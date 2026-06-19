#include <Blue/Memory/BlueNew.h>
#include <Blue/Memory/Invoker/RuntimeAllocationInvoker.h>
#include <Blue/Memory/MemorySystem.h>
#include <Blue/Memory/Pool/MemoryPoolRegistry.h>
#include <Blue/System/Log.h>
#include <Blue/System/Types.h>

#include <gtest/gtest.h>
#include <string.h>


namespace
{
struct LeakLogContext
{
  Blue::Uint32 ErrorEventCount = 0;
  Blue::Bool SawMemoryLeakError = false;
};

struct LeakTestObject
{
  BLUE_USE_MEMORY_POOL( Test )

  LeakTestObject( ) noexcept
      : Value( 0 )
  {}

  ~LeakTestObject( ) noexcept = default;

  Blue::Uint32 Value;
};

static void LeakLogSinkWrite( void* context, const Blue::LogEvent& event )
{
  LeakLogContext* leakContext = static_cast< LeakLogContext* >( context );

  if ( event.Level < Blue::LogLevel::Error )
  {
    return;
  }

  ++leakContext->ErrorEventCount;

  const bool isMemoryCategory =
    event.Category != nullptr && event.Category->Name != nullptr && strcmp( event.Category->Name, "LogMemory" ) == 0;

  const bool isLeakMessage = event.Message != nullptr && strstr( event.Message, "live allocation" ) != nullptr;

  if ( isMemoryCategory && isLeakMessage )
  {
    leakContext->SawMemoryLeakError = true;
  }
}

static void InitializeLeakTestLogger( LeakLogContext& context )
{
  ASSERT_TRUE( Blue::InitializeLogger( ) );

  Blue::LogSink sink = { };
  sink.Context = &context;
  sink.Write = &LeakLogSinkWrite;
  sink.MinimumLevel = Blue::LogLevel::Error;

  ASSERT_TRUE( Blue::RegisterLogSink( sink ) );
}

static void ShutdownLeakTestLogger( )
{
  Blue::ShutdownLogger( );
}

static Blue::MemorySystemDesc CreateLeakDetectionMemorySystemDesc( )
{
  Blue::MemorySystemDesc desc = { };
  desc.EnableLeakDetection = true;
  return desc;
}
} // namespace

static void BlueMemoryLeakDetection_DetectsTypedAllocationLeak( )
{
  LeakLogContext logContext = { };
  InitializeLeakTestLogger( logContext );

  Blue::MemorySystemDesc desc = CreateLeakDetectionMemorySystemDesc( );
  ASSERT_TRUE( Blue::InitializeMemorySystem( desc ).Succeeded( ) );

  Blue::MemoryPoolStats before = { };
  ASSERT_TRUE( Blue::CaptureMemoryPoolStats( Blue::MemoryPoolId::Test, before ) );

  LeakTestObject* object = Blue::BlueNew< LeakTestObject >( );
  ASSERT_TRUE( object != nullptr );

  Blue::MemoryPoolStats afterAllocate = { };
  ASSERT_TRUE( Blue::CaptureMemoryPoolStats( Blue::MemoryPoolId::Test, afterAllocate ) );

  ASSERT_TRUE( afterAllocate.CurrentBytes == before.CurrentBytes + sizeof( LeakTestObject ) );

  Blue::ShutdownMemorySystem( );

  ASSERT_TRUE( logContext.SawMemoryLeakError );

  ShutdownLeakTestLogger( );
}

static void BlueMemoryLeakDetection_DetectsRuntimeAllocationLeak( )
{
  LeakLogContext logContext = { };
  InitializeLeakTestLogger( logContext );

  constexpr Blue::Size byteSize = 128;
  constexpr Blue::Size alignment = 16;
  constexpr Blue::MemoryPoolId pool = Blue::MemoryPoolId::Test;
  constexpr Blue::AllocationTag tag = Blue::AllocationTag::Test;

  Blue::MemorySystemDesc desc = CreateLeakDetectionMemorySystemDesc( );
  ASSERT_TRUE( Blue::InitializeMemorySystem( desc ).Succeeded( ) );

  Blue::MemoryPoolStats before = { };
  ASSERT_TRUE( Blue::CaptureMemoryPoolStats( pool, before ) );

  Blue::AllocationRequest request = BLUE_POOL_ALLOCATION_REQUEST( byteSize, alignment, tag, pool );

  void* pointer = Blue::BlueTryAllocate( request );
  ASSERT_TRUE( pointer != nullptr );

  Blue::MemoryPoolStats afterAllocate = { };
  ASSERT_TRUE( Blue::CaptureMemoryPoolStats( pool, afterAllocate ) );

  ASSERT_TRUE( afterAllocate.CurrentBytes == before.CurrentBytes + byteSize );

  Blue::ShutdownMemorySystem( );

  ASSERT_TRUE( logContext.SawMemoryLeakError );

  ShutdownLeakTestLogger( );
}

TEST( BlueMemoryLeakDetectionTests, RunsSuccessfully )
{
  BlueMemoryLeakDetection_DetectsTypedAllocationLeak( );

  BlueMemoryLeakDetection_DetectsRuntimeAllocationLeak( );
}

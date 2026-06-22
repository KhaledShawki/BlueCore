#include <Blue/Memory/BlueNew.h>
#include <Blue/Memory/Config/BlueMemoryConfig.h>
#include <Blue/Memory/MemorySystem.h>
#include <Blue/Memory/Pool/MemoryPoolRegistry.h>
#include <Blue/System/Types.h>

#include <gtest/gtest.h>


namespace
{
struct RendererObject
{
  BLUE_USE_MEMORY_POOL( Renderer )

  RendererObject( ) noexcept { ++Constructed; }

  ~RendererObject( ) noexcept { ++Destroyed; }

  static Blue::Uint32 Constructed;
  static Blue::Uint32 Destroyed;
};

Blue::Uint32 RendererObject::Constructed = 0;
Blue::Uint32 RendererObject::Destroyed = 0;

struct ExplicitObject
{
  ExplicitObject( Blue::Uint32 value ) noexcept
      : Value( value )
  {}

  ~ExplicitObject( ) noexcept = default;
  Blue::Uint32 Value = 0;
};
} // namespace

TEST( BlueMemoryInvokerTests, RunsSuccessfully )
{
  Blue::MemorySystemDesc desc = { };
  ASSERT_TRUE( Blue::InitializeMemorySystem( desc ).Succeeded( ) );

#if BLUE_MEMORY_ENABLE_POOL_ACCOUNTING
  Blue::MemoryPoolStats before = { };
  ASSERT_TRUE( Blue::CaptureMemoryPoolStats( Blue::MemoryPoolId::Renderer, before ) );
#endif

  RendererObject* object = Blue::BlueNew< RendererObject >( );
  ASSERT_TRUE( object != nullptr );
  ASSERT_TRUE( RendererObject::Constructed == 1 );
  ASSERT_TRUE( RendererObject::Destroyed == 0 );

#if BLUE_MEMORY_ENABLE_POOL_ACCOUNTING
  Blue::MemoryPoolStats during = { };
  ASSERT_TRUE( Blue::CaptureMemoryPoolStats( Blue::MemoryPoolId::Renderer, during ) );
  ASSERT_TRUE( during.CurrentBytes >= before.CurrentBytes + sizeof( RendererObject ) );
  ASSERT_TRUE( during.AllocationCount == before.AllocationCount + 1 );
#endif

  Blue::BlueDelete( object );
  ASSERT_TRUE( RendererObject::Destroyed == 1 );

#if BLUE_MEMORY_ENABLE_POOL_ACCOUNTING
  Blue::MemoryPoolStats after = { };
  ASSERT_TRUE( Blue::CaptureMemoryPoolStats( Blue::MemoryPoolId::Renderer, after ) );
  ASSERT_TRUE( after.CurrentBytes == before.CurrentBytes );
  ASSERT_TRUE( after.FreeCount == before.FreeCount + 1 );
  ASSERT_TRUE( after.PeakBytes >= during.CurrentBytes );
#endif

  ExplicitObject* explicitObject = Blue::BlueNewInPool< Blue::MemoryPoolId::Test, ExplicitObject >( 42 );
  ASSERT_TRUE( explicitObject != nullptr );
  ASSERT_TRUE( explicitObject->Value == 42 );
  Blue::BlueDeleteFromPool< Blue::MemoryPoolId::Test >( explicitObject );

  Blue::ShutdownMemorySystem( );
}

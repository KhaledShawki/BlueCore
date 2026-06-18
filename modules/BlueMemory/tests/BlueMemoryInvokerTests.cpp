#include <Blue/Memory/BlueNew.h>
#include <Blue/Memory/MemorySystem.h>
#include <Blue/Memory/Pool/MemoryPoolRegistry.h>
#include <Blue/System/Types.h>

#include <stdio.h>
#include <stdlib.h>

#define BLUE_TEST_EXPECT( expression )                                                                                 \
  do                                                                                                                   \
  {                                                                                                                    \
    if ( !( expression ) )                                                                                             \
    {                                                                                                                  \
      fprintf( stderr, "Test failed: %s at %s:%d\n", #expression, __FILE__, __LINE__ );                                \
      abort( );                                                                                                        \
    }                                                                                                                  \
  }                                                                                                                    \
  while ( false )

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

int main( )
{
  Blue::MemorySystemDesc desc = { };
  BLUE_TEST_EXPECT( Blue::InitializeMemorySystem( desc ).Succeeded( ) );

  Blue::MemoryPoolStats before = { };
  BLUE_TEST_EXPECT( Blue::CaptureMemoryPoolStats( Blue::MemoryPoolId::Renderer, before ) );

  RendererObject* object = Blue::BlueNew< RendererObject >( );
  BLUE_TEST_EXPECT( object != nullptr );
  BLUE_TEST_EXPECT( RendererObject::Constructed == 1 );
  BLUE_TEST_EXPECT( RendererObject::Destroyed == 0 );

  Blue::MemoryPoolStats during = { };
  BLUE_TEST_EXPECT( Blue::CaptureMemoryPoolStats( Blue::MemoryPoolId::Renderer, during ) );
  BLUE_TEST_EXPECT( during.CurrentBytes >= before.CurrentBytes + sizeof( RendererObject ) );
  BLUE_TEST_EXPECT( during.AllocationCount == before.AllocationCount + 1 );

  Blue::BlueDelete( object );
  BLUE_TEST_EXPECT( RendererObject::Destroyed == 1 );

  Blue::MemoryPoolStats after = { };
  BLUE_TEST_EXPECT( Blue::CaptureMemoryPoolStats( Blue::MemoryPoolId::Renderer, after ) );
  BLUE_TEST_EXPECT( after.CurrentBytes == before.CurrentBytes );
  BLUE_TEST_EXPECT( after.FreeCount == before.FreeCount + 1 );
  BLUE_TEST_EXPECT( after.PeakBytes >= during.CurrentBytes );

  ExplicitObject* explicitObject = Blue::BlueNewInPool< Blue::MemoryPoolId::Test, ExplicitObject >( 42 );
  BLUE_TEST_EXPECT( explicitObject != nullptr );
  BLUE_TEST_EXPECT( explicitObject->Value == 42 );
  Blue::BlueDeleteFromPool< Blue::MemoryPoolId::Test >( explicitObject );

  Blue::ShutdownMemorySystem( );
  printf( "BlueMemory invoker tests passed.\n" );
  return 0;
}

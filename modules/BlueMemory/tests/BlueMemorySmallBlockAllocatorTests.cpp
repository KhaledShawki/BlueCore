#include <Blue/Memory/Allocator/SmallBlockAllocator.h>
#include <Blue/Memory/BlueNew.h>
#include <Blue/Memory/Config/BlueMemoryConfig.h>
#include <Blue/Memory/Invoker/RuntimeAllocationInvoker.h>
#include <Blue/Memory/MemorySystem.h>
#include <Blue/Memory/Pool/MemoryPoolRegistry.h>
#include <Blue/System/Types.h>

#include <gtest/gtest.h>


namespace
{
struct SmallRendererObject
{
  BLUE_USE_MEMORY_POOL( Renderer )

  SmallRendererObject( Blue::Uint32 value ) noexcept
      : Value( value )
  {}

  ~SmallRendererObject( ) noexcept { Value = 0; }

  Blue::Uint32 Value = 0;
};

static_assert( sizeof( SmallRendererObject ) <= Blue::BlueSmallBlockMaxSize );
} // namespace

TEST( BlueMemorySmallBlockAllocatorTests, RunsSuccessfully )
{
  Blue::MemorySystemDesc desc = { };
  ASSERT_TRUE( Blue::InitializeMemorySystem( desc ).Succeeded( ) );

  ASSERT_TRUE( Blue::IsSmallBlockAllocationSupported( 1, 1 ) );
  ASSERT_TRUE( Blue::IsSmallBlockAllocationSupported( 64, 64 ) );
  ASSERT_TRUE( Blue::IsSmallBlockAllocationSupported( 128, 16 ) );
  ASSERT_TRUE( !Blue::IsSmallBlockAllocationSupported( Blue::BlueSmallBlockMaxSize + 1, 16 ) );
  ASSERT_TRUE( !Blue::IsSmallBlockAllocationSupported( 64, 3 ) );
  ASSERT_TRUE( Blue::GetSmallBlockClassSize( 17, 8 ) == 32 );
  ASSERT_TRUE( Blue::GetSmallBlockClassSize( 32, 64 ) == 64 );

  Blue::SmallBlockAllocatorStats initialStats = Blue::GetSmallBlockAllocatorStats( );
  SmallRendererObject* objects[ 256 ] = { };

#if BLUE_MEMORY_ENABLE_POOL_ACCOUNTING
  Blue::MemoryPoolStats before = { };
  ASSERT_TRUE( Blue::CaptureMemoryPoolStats( Blue::MemoryPoolId::Renderer, before ) );
#endif

  for ( Blue::Size index = 0; index < 256; ++index )
  {
    objects[ index ] = Blue::BlueNew< SmallRendererObject >( static_cast< Blue::Uint32 >( index ) );
    ASSERT_TRUE( objects[ index ] != nullptr );
    ASSERT_TRUE( objects[ index ]->Value == index );
    ASSERT_TRUE( ( reinterpret_cast< Blue::NativeUInt >( objects[ index ] ) % alignof( SmallRendererObject ) ) == 0 );
  }

#if BLUE_MEMORY_ENABLE_POOL_ACCOUNTING
  Blue::MemoryPoolStats during = { };
  ASSERT_TRUE( Blue::CaptureMemoryPoolStats( Blue::MemoryPoolId::Renderer, during ) );
  ASSERT_TRUE( during.CurrentBytes == before.CurrentBytes + sizeof( SmallRendererObject ) * 256 );
  ASSERT_TRUE( during.AllocationCount == before.AllocationCount + 256 );
#endif

  for ( Blue::Size index = 0; index < 256; ++index )
  {
    Blue::BlueDelete( objects[ index ] );
  }

#if BLUE_MEMORY_ENABLE_POOL_ACCOUNTING
  Blue::MemoryPoolStats after = { };
  ASSERT_TRUE( Blue::CaptureMemoryPoolStats( Blue::MemoryPoolId::Renderer, after ) );
  ASSERT_TRUE( after.CurrentBytes == before.CurrentBytes );
  ASSERT_TRUE( after.FreeCount == before.FreeCount + 256 );
#endif

  Blue::SmallBlockAllocatorStats finalStats = Blue::GetSmallBlockAllocatorStats( );
  ASSERT_TRUE( finalStats.AllocateCount >= initialStats.AllocateCount + 256 );
  ASSERT_TRUE( finalStats.FreeCount >= initialStats.FreeCount + 256 );
  ASSERT_TRUE( finalStats.RefillCount > initialStats.RefillCount );
  ASSERT_TRUE( finalStats.SlabCount > initialStats.SlabCount );

  Blue::AllocationRequest request =
    BLUE_POOL_ALLOCATION_REQUEST( 32, 16, Blue::AllocationTag::Test, Blue::MemoryPoolId::Resources );
  void* runtimePointer = Blue::BlueTryAllocate( request );
  ASSERT_TRUE( runtimePointer != nullptr );
  ASSERT_TRUE( ( reinterpret_cast< Blue::NativeUInt >( runtimePointer ) % 16 ) == 0 );
  Blue::BlueFree(
    Blue::AllocationFreeRequest{ runtimePointer, 32, 16, Blue::MemoryPoolId::Resources, Blue::AllocationTag::Test } );

  Blue::ShutdownMemorySystem( );
}

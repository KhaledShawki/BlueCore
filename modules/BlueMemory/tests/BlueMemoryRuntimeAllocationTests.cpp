#include <Blue/Memory/Allocation/AllocationValidation.h>
#include <Blue/Memory/Config/BlueMemoryConfig.h>
#include <Blue/Memory/Invoker/RuntimeAllocationInvoker.h>
#include <Blue/Memory/MemorySystem.h>
#include <Blue/Memory/Oom/OomReporter.h>
#include <Blue/Memory/Pool/MemoryPoolRegistry.h>
#include <Blue/Memory/PoolAllocator.h>
#include <Blue/System/Types.h>

#include <cstddef>

#include <gtest/gtest.h>


namespace
{
struct MemorySystemTestScope
{
  MemorySystemTestScope( )
  {
    Blue::MemorySystemDesc desc = { };
    desc.OomReportBuffer = Reports;
    desc.OomReportCapacity = ReportCapacity;

    Initialized = Blue::InitializeMemorySystem( desc ).Succeeded( );
  }

  ~MemorySystemTestScope( )
  {
    if ( Initialized )
    {
      Blue::ShutdownMemorySystem( );
    }
  }

  static constexpr Blue::Size ReportCapacity = 8;

  Blue::OomReport Reports[ ReportCapacity ] = { };
  bool Initialized = false;
};
} // namespace
static void VerifyHeapReallocatePreservesExistingBytes( )
{
  Blue::Allocator allocator = Blue::GetDefaultAllocator( );
  Blue::AllocationResult initial =
    Blue::Allocate( allocator, BLUE_ALLOCATION_REQUEST( 64, 16, Blue::AllocationTag::Test ) );
  ASSERT_TRUE( initial.Pointer != nullptr );

  Blue::Byte* bytes = static_cast< Blue::Byte* >( initial.Pointer );
  for ( Blue::Size index = 0; index < 64; ++index )
  {
    bytes[ index ] = static_cast< Blue::Byte >( index + 1 );
  }

  Blue::AllocationResult grown =
    Blue::Reallocate( allocator, initial.Pointer, 64, BLUE_ALLOCATION_REQUEST( 128, 16, Blue::AllocationTag::Test ) );
  ASSERT_TRUE( grown.Pointer != nullptr );

  Blue::Byte* grownBytes = static_cast< Blue::Byte* >( grown.Pointer );
  for ( Blue::Size index = 0; index < 64; ++index )
  {
    ASSERT_TRUE( grownBytes[ index ] == static_cast< Blue::Byte >( index + 1 ) );
  }

  Blue::Free( allocator, grown.Pointer, grown.ByteSize, 16 );
}

static void VerifyAllocatorFreeUsesRequestPool( )
{
  Blue::Allocator allocator = Blue::GetDefaultAllocator( );

#if BLUE_MEMORY_ENABLE_POOL_ACCOUNTING
  Blue::MemoryPoolStats before = { };
  ASSERT_TRUE( Blue::CaptureMemoryPoolStats( Blue::MemoryPoolId::Resources, before ) );
#endif

  Blue::AllocationResult allocation =
    Blue::Allocate( allocator,
                    BLUE_POOL_ALLOCATION_REQUEST( 96, 16, Blue::AllocationTag::Test, Blue::MemoryPoolId::Resources ) );
  ASSERT_TRUE( allocation.Pointer != nullptr );

#if BLUE_MEMORY_ENABLE_POOL_ACCOUNTING
  Blue::MemoryPoolStats during = { };
  ASSERT_TRUE( Blue::CaptureMemoryPoolStats( Blue::MemoryPoolId::Resources, during ) );
  ASSERT_TRUE( during.CurrentBytes == before.CurrentBytes + 96 );
#endif

  Blue::Free( allocator,
              Blue::AllocationFreeRequest{ allocation.Pointer,
                                           allocation.ByteSize,
                                           16,
                                           Blue::MemoryPoolId::Resources,
                                           Blue::AllocationTag::Test } );

#if BLUE_MEMORY_ENABLE_POOL_ACCOUNTING
  Blue::MemoryPoolStats after = { };
  ASSERT_TRUE( Blue::CaptureMemoryPoolStats( Blue::MemoryPoolId::Resources, after ) );
  ASSERT_TRUE( after.CurrentBytes == before.CurrentBytes );
#endif
}

static void VerifyPoolAllocatorAlignmentAndBoundsHardening( )
{
  Blue::PoolAllocator pool;
  Blue::PoolAllocatorDesc poolDesc = { };
  poolDesc.ObjectSize = 24;
  poolDesc.ObjectAlignment = 32;
  poolDesc.ObjectCount = 2;
  poolDesc.Tag = Blue::AllocationTag::Test;
  poolDesc.BackingAllocator = Blue::GetDefaultAllocator( );

  ASSERT_TRUE( pool.Initialize( poolDesc ) );
  ASSERT_TRUE( pool.GetObjectSize( ) == 24 );
  ASSERT_TRUE( pool.GetObjectStride( ) == 32 );

  Blue::AllocationResult first = pool.Allocate( BLUE_ALLOCATION_REQUEST( 24, 32, Blue::AllocationTag::Test ) );
  Blue::AllocationResult second = pool.Allocate( BLUE_ALLOCATION_REQUEST( 24, 32, Blue::AllocationTag::Test ) );
  ASSERT_TRUE( first.Pointer != nullptr );
  ASSERT_TRUE( second.Pointer != nullptr );
  ASSERT_TRUE( ( reinterpret_cast< Blue::NativeUInt >( first.Pointer ) % 32 ) == 0 );
  ASSERT_TRUE( ( reinterpret_cast< Blue::NativeUInt >( second.Pointer ) % 32 ) == 0 );

  Blue::AllocationResult exhausted = pool.Allocate( BLUE_ALLOCATION_REQUEST( 24, 32, Blue::AllocationTag::Test ) );
  ASSERT_TRUE( exhausted.Pointer == nullptr );
  ASSERT_TRUE( pool.GetUsedCount( ) == 2 );

  pool.Free( first.Pointer, first.ByteSize, 32 );
  pool.Free( second.Pointer, second.ByteSize, 32 );
  ASSERT_TRUE( pool.GetUsedCount( ) == 0 );

  pool.Shutdown( );
}

TEST( BlueMemoryRuntimeAllocationTests, RunsSuccessfully )
{
  Blue::OomReport reports[ 8 ] = { };
  Blue::MemorySystemDesc desc = { };
  desc.OomReportBuffer = reports;
  desc.OomReportCapacity = 8;
  ASSERT_TRUE( Blue::InitializeMemorySystem( desc ).Succeeded( ) );

  VerifyHeapReallocatePreservesExistingBytes( );
  VerifyAllocatorFreeUsesRequestPool( );
  VerifyPoolAllocatorAlignmentAndBoundsHardening( );

  Blue::AllocationRequest request =
    BLUE_POOL_ALLOCATION_REQUEST( 128, 16, Blue::AllocationTag::Test, Blue::MemoryPoolId::Resources );
  void* pointer = Blue::BlueTryAllocate( request );
  ASSERT_TRUE( pointer != nullptr );

#if BLUE_MEMORY_ENABLE_POOL_ACCOUNTING
  Blue::MemoryPoolStats during = { };
  ASSERT_TRUE( Blue::CaptureMemoryPoolStats( Blue::MemoryPoolId::Resources, during ) );
  ASSERT_TRUE( during.CurrentBytes >= 128 );
  ASSERT_TRUE( during.AllocationCount >= 1 );
#endif

  Blue::BlueFree(
    Blue::AllocationFreeRequest{ pointer, 128, 16, Blue::MemoryPoolId::Resources, Blue::AllocationTag::Test } );

  Blue::AllocationRequest invalid =
    BLUE_POOL_ALLOCATION_REQUEST( 64, 3, Blue::AllocationTag::Test, Blue::MemoryPoolId::Resources );
  void* invalidPointer = Blue::BlueTryAllocate( invalid );
  ASSERT_TRUE( invalidPointer == nullptr );

#if BLUE_MEMORY_ENABLE_OOM_REPORTS
  Blue::OomReport captured[ 8 ] = { };
  const Blue::Size reportCount = Blue::CaptureOomReports( captured, 8 );
  ASSERT_TRUE( reportCount > 0 );
  ASSERT_TRUE( captured[ 0 ].Reason == Blue::AllocationFailureReason::InvalidAlignment );
  ASSERT_TRUE( captured[ 0 ].NativeThreadId != 0 );
#endif

  Blue::ShutdownMemorySystem( );
}

TEST( BlueMemoryRuntimeAllocationTests, RejectsZeroAlignment )
{
  Blue::AllocationRequest request =
    BLUE_POOL_ALLOCATION_REQUEST( 64, 0, Blue::AllocationTag::Test, Blue::MemoryPoolId::Test );

  void* pointer = Blue::BlueTryAllocate( request );

  EXPECT_EQ( pointer, nullptr );
}

TEST( BlueMemoryRuntimeAllocationTests, RejectsNonPowerOfTwoAlignment )
{
  Blue::AllocationRequest request =
    BLUE_POOL_ALLOCATION_REQUEST( 64, 3, Blue::AllocationTag::Test, Blue::MemoryPoolId::Test );

  void* pointer = Blue::BlueTryAllocate( request );

  EXPECT_EQ( pointer, nullptr );
}

TEST( BlueMemoryRuntimeAllocationTests, NormalizesSmallPowerOfTwoAlignment )
{
  MemorySystemTestScope memory;
  ASSERT_TRUE( memory.Initialized );

  Blue::AllocationRequest request =
    BLUE_POOL_ALLOCATION_REQUEST( 64, 1, Blue::AllocationTag::Test, Blue::MemoryPoolId::Test );

  void* pointer = Blue::BlueTryAllocate( request );

  ASSERT_NE( pointer, nullptr );

  Blue::BlueFree( Blue::AllocationFreeRequest{ pointer,
                                               64,
                                               Blue::MinimumAllocationAlignment( ),
                                               Blue::MemoryPoolId::Test,
                                               Blue::AllocationTag::Test } );
}

TEST( BlueMemoryRuntimeAllocationTests, HeapReallocateToZeroFreesAllocation )
{
  MemorySystemTestScope memory;
  ASSERT_TRUE( memory.Initialized );

  Blue::Allocator allocator = Blue::GetDefaultAllocator( );

  Blue::AllocationResult allocation =
    Blue::Allocate( allocator,
                    BLUE_POOL_ALLOCATION_REQUEST( 64, 16, Blue::AllocationTag::Test, Blue::MemoryPoolId::Test ) );

  ASSERT_NE( allocation.Pointer, nullptr );

  Blue::MemoryPoolStats beforeReallocate = { };
  ASSERT_TRUE( Blue::CaptureMemoryPoolStats( Blue::MemoryPoolId::Test, beforeReallocate ) );
#if BLUE_MEMORY_ENABLE_POOL_ACCOUNTING
  ASSERT_EQ( beforeReallocate.CurrentBytes, 64 );
#else
  static_cast< void >( beforeReallocate );
#endif

  Blue::AllocationResult reallocated =
    Blue::Reallocate( allocator,
                      allocation.Pointer,
                      allocation.ByteSize,
                      BLUE_POOL_ALLOCATION_REQUEST( 0, 16, Blue::AllocationTag::Test, Blue::MemoryPoolId::Test ) );

  EXPECT_EQ( reallocated.Pointer, nullptr );
  EXPECT_EQ( reallocated.ByteSize, 0 );

  Blue::MemoryPoolStats afterReallocate = { };
  ASSERT_TRUE( Blue::CaptureMemoryPoolStats( Blue::MemoryPoolId::Test, afterReallocate ) );
#if BLUE_MEMORY_ENABLE_POOL_ACCOUNTING
  EXPECT_EQ( afterReallocate.CurrentBytes, 0 );
  EXPECT_EQ( afterReallocate.FreeCount, beforeReallocate.FreeCount + 1 );
#else
  static_cast< void >( afterReallocate );
#endif
}

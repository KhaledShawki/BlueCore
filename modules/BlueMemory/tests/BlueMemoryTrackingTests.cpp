#include <Blue/Memory/Invoker/RuntimeAllocationInvoker.h>
#include <Blue/Memory/MemorySystem.h>
#include <Blue/Memory/Pool/MemoryPoolRegistry.h>
#include <Blue/Memory/Tracking/MemoryAllocationTracker.h>

#include <gtest/gtest.h>

namespace
{
class BlueMemoryTrackingTests : public testing::Test
{
protected:
  void TearDown( ) override
  {
    if ( Blue::IsMemorySystemInitialized( ) )
    {
      Blue::ShutdownMemorySystem( );
    }
  }
};

Blue::MemorySystemDesc MakeTrackingMemorySystemDesc( ) noexcept
{
  Blue::MemorySystemDesc desc = { };
  desc.EnableTracking = true;
  desc.EnableLeakDetection = false;
  desc.TrackingCapacity = 64;
  return desc;
}
} // namespace

TEST_F( BlueMemoryTrackingTests, CompileTimeConfigurationMatchesBuildProfile )
{
#if BLUE_MEMORY_ENABLE_TRACKING
  EXPECT_TRUE( Blue::IsMemoryAllocationTrackingCompiledIn( ) );
#else
  EXPECT_FALSE( Blue::IsMemoryAllocationTrackingCompiledIn( ) );
#endif
}

TEST_F( BlueMemoryTrackingTests, TrackingIsDisabledByDefault )
{
  Blue::MemorySystemDesc desc = { };
  ASSERT_TRUE( Blue::InitializeMemorySystem( desc ).Succeeded( ) );

  EXPECT_FALSE( Blue::IsMemoryAllocationTrackingEnabled( ) );

  const Blue::MemoryAllocationTrackerStats stats = Blue::GetMemoryAllocationTrackerStats( );
  EXPECT_FALSE( stats.Enabled );
  EXPECT_EQ( stats.Capacity, 0u );
  EXPECT_EQ( stats.ActiveCount, 0u );
}

#if BLUE_MEMORY_ENABLE_TRACKING
TEST_F( BlueMemoryTrackingTests, TrackingCapturesAndRemovesRuntimeAllocations )
{
  ASSERT_TRUE( Blue::InitializeMemorySystem( MakeTrackingMemorySystemDesc( ) ).Succeeded( ) );
  ASSERT_TRUE( Blue::IsMemoryAllocationTrackingEnabled( ) );

  constexpr Blue::Size byteSize = 96;
  constexpr Blue::Size alignment = 16;
  constexpr Blue::MemoryPoolId pool = Blue::MemoryPoolId::Test;
  constexpr Blue::AllocationTag tag = Blue::AllocationTag::Test;

  Blue::MemoryPoolStats before = { };
  ASSERT_TRUE( Blue::CaptureMemoryPoolStats( pool, before ) );

  Blue::AllocationRequest request = BLUE_POOL_ALLOCATION_REQUEST( byteSize, alignment, tag, pool );
  void* pointer = Blue::BlueTryAllocate( request );
  ASSERT_NE( pointer, nullptr );

  Blue::MemoryAllocationRecord record = { };
  ASSERT_TRUE( Blue::TryFindTrackedMemoryAllocation( pointer, record ) );
  EXPECT_EQ( record.Pointer, pointer );
  EXPECT_EQ( record.ByteSize, byteSize );
  EXPECT_EQ( record.Alignment, alignment );
  EXPECT_EQ( record.Pool, pool );
  EXPECT_EQ( record.Tag, tag );
  EXPECT_EQ( record.Allocator, Blue::AllocatorKind::Default );
  EXPECT_NE( record.Location.File, nullptr );
  EXPECT_NE( record.Location.Function, nullptr );
  EXPECT_NE( record.Location.Line, 0u );

  const Blue::MemoryAllocationTrackerStats duringStats = Blue::GetMemoryAllocationTrackerStats( );
  EXPECT_TRUE( duringStats.Enabled );
  EXPECT_GE( duringStats.Capacity, 64u );
  EXPECT_EQ( duringStats.ActiveCount, 1u );
  EXPECT_EQ( duringStats.TotalTrackedCount, 1u );
  EXPECT_EQ( duringStats.TotalUntrackedCount, 0u );

  Blue::MemoryPoolStats during = { };
  ASSERT_TRUE( Blue::CaptureMemoryPoolStats( pool, during ) );
  EXPECT_EQ( during.CurrentBytes, before.CurrentBytes + byteSize );

  Blue::BlueFree( Blue::AllocationFreeRequest{ pointer, byteSize, alignment, pool, tag } );

  Blue::MemoryAllocationRecord removedRecord = { };
  EXPECT_FALSE( Blue::TryFindTrackedMemoryAllocation( pointer, removedRecord ) );

  const Blue::MemoryAllocationTrackerStats afterStats = Blue::GetMemoryAllocationTrackerStats( );
  EXPECT_EQ( afterStats.ActiveCount, 0u );
  EXPECT_EQ( afterStats.TotalTrackedCount, 1u );
  EXPECT_EQ( afterStats.TotalUntrackedCount, 1u );
  EXPECT_EQ( afterStats.UnknownFreeCount, 0u );

  Blue::MemoryPoolStats after = { };
  ASSERT_TRUE( Blue::CaptureMemoryPoolStats( pool, after ) );
  EXPECT_EQ( after.CurrentBytes, before.CurrentBytes );
}

TEST_F( BlueMemoryTrackingTests, RuntimeFreeUsesTrackedMetadataWhenCallerProvidesWrongMetadata )
{
  ASSERT_TRUE( Blue::InitializeMemorySystem( MakeTrackingMemorySystemDesc( ) ).Succeeded( ) );

  constexpr Blue::Size byteSize = 128;
  constexpr Blue::Size alignment = 16;
  constexpr Blue::MemoryPoolId pool = Blue::MemoryPoolId::Resources;
  constexpr Blue::AllocationTag tag = Blue::AllocationTag::Test;

  Blue::MemoryPoolStats beforeResources = { };
  Blue::MemoryPoolStats beforeSystem = { };
  ASSERT_TRUE( Blue::CaptureMemoryPoolStats( Blue::MemoryPoolId::Resources, beforeResources ) );
  ASSERT_TRUE( Blue::CaptureMemoryPoolStats( Blue::MemoryPoolId::System, beforeSystem ) );

  void* pointer = Blue::BlueTryAllocate( BLUE_POOL_ALLOCATION_REQUEST( byteSize, alignment, tag, pool ) );
  ASSERT_NE( pointer, nullptr );

  Blue::MemoryAllocationRecord record = { };
  ASSERT_TRUE( Blue::TryFindTrackedMemoryAllocation( pointer, record ) );
  ASSERT_EQ( record.Pool, pool );

  Blue::BlueFree(
    Blue::AllocationFreeRequest{ pointer, 1, 1, Blue::MemoryPoolId::System, Blue::AllocationTag::Unknown } );

  EXPECT_FALSE( Blue::TryFindTrackedMemoryAllocation( pointer, record ) );

  Blue::MemoryPoolStats afterResources = { };
  Blue::MemoryPoolStats afterSystem = { };
  ASSERT_TRUE( Blue::CaptureMemoryPoolStats( Blue::MemoryPoolId::Resources, afterResources ) );
  ASSERT_TRUE( Blue::CaptureMemoryPoolStats( Blue::MemoryPoolId::System, afterSystem ) );

  EXPECT_EQ( afterResources.CurrentBytes, beforeResources.CurrentBytes );
  EXPECT_EQ( afterResources.FreeCount, beforeResources.FreeCount + 1 );
  EXPECT_EQ( afterSystem.CurrentBytes, beforeSystem.CurrentBytes );
}

TEST_F( BlueMemoryTrackingTests, CaptureLiveMemoryAllocationsReportsActiveRecords )
{
  ASSERT_TRUE( Blue::InitializeMemorySystem( MakeTrackingMemorySystemDesc( ) ).Succeeded( ) );

  constexpr Blue::Size byteSize = 64;
  constexpr Blue::Size alignment = 16;
  constexpr Blue::MemoryPoolId pool = Blue::MemoryPoolId::Temporary;
  constexpr Blue::AllocationTag tag = Blue::AllocationTag::Test;

  void* pointer = Blue::BlueTryAllocate( BLUE_POOL_ALLOCATION_REQUEST( byteSize, alignment, tag, pool ) );
  ASSERT_NE( pointer, nullptr );

  Blue::MemoryAllocationRecord records[ 4 ] = { };
  const Blue::Size recordCount = Blue::CaptureLiveMemoryAllocations( records, 4 );
  ASSERT_EQ( recordCount, 1u );
  EXPECT_EQ( records[ 0 ].Pointer, pointer );
  EXPECT_EQ( records[ 0 ].ByteSize, byteSize );
  EXPECT_EQ( records[ 0 ].Alignment, alignment );
  EXPECT_EQ( records[ 0 ].Pool, pool );
  EXPECT_EQ( records[ 0 ].Tag, tag );

  Blue::BlueFree( Blue::AllocationFreeRequest{ pointer, byteSize, alignment, pool, tag } );
  EXPECT_EQ( Blue::CaptureLiveMemoryAllocations( records, 4 ), 0u );
}

TEST_F( BlueMemoryTrackingTests, NoTrackingFlagSkipsRuntimeTracking )
{
  ASSERT_TRUE( Blue::InitializeMemorySystem( MakeTrackingMemorySystemDesc( ) ).Succeeded( ) );

  Blue::AllocationRequest request =
    BLUE_POOL_ALLOCATION_REQUEST( 64, 16, Blue::AllocationTag::Test, Blue::MemoryPoolId::Test );
  request.Flags = Blue::AllocationFlag_NoTracking;

  void* pointer = Blue::BlueTryAllocate( request );
  ASSERT_NE( pointer, nullptr );

  Blue::MemoryAllocationRecord record = { };
  EXPECT_FALSE( Blue::TryFindTrackedMemoryAllocation( pointer, record ) );

  Blue::BlueFree(
    Blue::AllocationFreeRequest{ pointer, request.ByteSize, request.Alignment, request.Pool, request.Tag } );
}
#else
TEST_F( BlueMemoryTrackingTests, TrackingRequestIsIgnoredWhenTrackingIsCompiledOut )
{
  ASSERT_TRUE( Blue::InitializeMemorySystem( MakeTrackingMemorySystemDesc( ) ).Succeeded( ) );

  EXPECT_FALSE( Blue::IsMemoryAllocationTrackingEnabled( ) );

  const Blue::MemoryAllocationTrackerStats stats = Blue::GetMemoryAllocationTrackerStats( );
  EXPECT_FALSE( stats.Enabled );
  EXPECT_EQ( stats.Capacity, 0u );
  EXPECT_EQ( stats.ActiveCount, 0u );
}
#endif

TEST_F( BlueMemoryTrackingTests, ZeroMemoryFlagClearsRuntimeAllocations )
{
  ASSERT_TRUE( Blue::InitializeMemorySystem( MakeTrackingMemorySystemDesc( ) ).Succeeded( ) );

  Blue::AllocationRequest request =
    BLUE_POOL_ALLOCATION_REQUEST( 128, 16, Blue::AllocationTag::Test, Blue::MemoryPoolId::Test );
  request.Flags = Blue::AllocationFlag_ZeroMemory;

  void* pointer = Blue::BlueTryAllocate( request );
  ASSERT_NE( pointer, nullptr );

  const Blue::Byte* bytes = static_cast< const Blue::Byte* >( pointer );
  for ( Blue::Size index = 0; index < request.ByteSize; ++index )
  {
    EXPECT_EQ( bytes[ index ], static_cast< Blue::Byte >( 0 ) ) << "index=" << index;
  }

  Blue::BlueFree(
    Blue::AllocationFreeRequest{ pointer, request.ByteSize, request.Alignment, request.Pool, request.Tag } );
}

#include <Blue/Memory/Backend/MemoryBackend.h>
#include <Blue/Memory/BlueNew.h>
#include <Blue/Memory/Config/BlueMemoryConfig.h>
#include <Blue/Memory/Invoker/RuntimeAllocationInvoker.h>
#include <Blue/Memory/MemorySystem.h>
#include <Blue/Memory/Pool/MemoryPoolRegistry.h>
#include <Blue/System/Types.h>

#include <gtest/gtest.h>


namespace
{
struct ContractTestObject
{
  BLUE_USE_MEMORY_POOL( Test )

  ContractTestObject( ) noexcept
      : Value( 0 )
  {}

  ~ContractTestObject( ) noexcept = default;

  Blue::Uint32 Value;
};

struct AllPoolStats
{
  Blue::MemoryPoolStats System = { };
  Blue::MemoryPoolStats Renderer = { };
  Blue::MemoryPoolStats Resources = { };
  Blue::MemoryPoolStats JobSystem = { };
  Blue::MemoryPoolStats Temporary = { };
  Blue::MemoryPoolStats Test = { };
};
} // namespace


static void ExpectPoolStatsEqual( const Blue::MemoryPoolStats& actual, const Blue::MemoryPoolStats& expected )
{
  ASSERT_TRUE( actual.CurrentBytes == expected.CurrentBytes );
  ASSERT_TRUE( actual.PeakBytes == expected.PeakBytes );
  ASSERT_TRUE( actual.TotalAllocatedBytes == expected.TotalAllocatedBytes );
  ASSERT_TRUE( actual.TotalFreedBytes == expected.TotalFreedBytes );
  ASSERT_TRUE( actual.AllocationCount == expected.AllocationCount );
  ASSERT_TRUE( actual.FreeCount == expected.FreeCount );
  ASSERT_TRUE( actual.FailedAllocationCount == expected.FailedAllocationCount );
  ASSERT_TRUE( actual.BudgetExceededCount == expected.BudgetExceededCount );
}

static void CheckPoolStatsAreZero( Blue::MemoryPoolId poolId )
{
  Blue::MemoryPoolStats poolStats = { };

  ASSERT_TRUE( Blue::CaptureMemoryPoolStats( poolId, poolStats ) );

  ASSERT_TRUE( poolStats.CurrentBytes == 0 );
  ASSERT_TRUE( poolStats.PeakBytes == 0 );
  ASSERT_TRUE( poolStats.TotalAllocatedBytes == 0 );
  ASSERT_TRUE( poolStats.TotalFreedBytes == 0 );
  ASSERT_TRUE( poolStats.AllocationCount == 0 );
  ASSERT_TRUE( poolStats.FreeCount == 0 );
  ASSERT_TRUE( poolStats.FailedAllocationCount == 0 );
  ASSERT_TRUE( poolStats.BudgetExceededCount == 0 );
}

static void CaptureAllPoolStats( AllPoolStats& stats )
{
  ASSERT_TRUE( Blue::CaptureMemoryPoolStats( Blue::MemoryPoolId::System, stats.System ) );
  ASSERT_TRUE( Blue::CaptureMemoryPoolStats( Blue::MemoryPoolId::Renderer, stats.Renderer ) );
  ASSERT_TRUE( Blue::CaptureMemoryPoolStats( Blue::MemoryPoolId::Resources, stats.Resources ) );
  ASSERT_TRUE( Blue::CaptureMemoryPoolStats( Blue::MemoryPoolId::JobSystem, stats.JobSystem ) );
  ASSERT_TRUE( Blue::CaptureMemoryPoolStats( Blue::MemoryPoolId::Temporary, stats.Temporary ) );
  ASSERT_TRUE( Blue::CaptureMemoryPoolStats( Blue::MemoryPoolId::Test, stats.Test ) );
}

static void ExpectAllPoolStatsEqual( const AllPoolStats& actual, const AllPoolStats& expected )
{
  ExpectPoolStatsEqual( actual.System, expected.System );
  ExpectPoolStatsEqual( actual.Renderer, expected.Renderer );
  ExpectPoolStatsEqual( actual.Resources, expected.Resources );
  ExpectPoolStatsEqual( actual.JobSystem, expected.JobSystem );
  ExpectPoolStatsEqual( actual.Temporary, expected.Temporary );
  ExpectPoolStatsEqual( actual.Test, expected.Test );
}

static void BlueMemoryContract_PoolStatsAreZeroAfterInitialization( )
{
  CheckPoolStatsAreZero( Blue::MemoryPoolId::System );
  CheckPoolStatsAreZero( Blue::MemoryPoolId::Renderer );
  CheckPoolStatsAreZero( Blue::MemoryPoolId::Resources );
  CheckPoolStatsAreZero( Blue::MemoryPoolId::JobSystem );
  CheckPoolStatsAreZero( Blue::MemoryPoolId::Temporary );
  CheckPoolStatsAreZero( Blue::MemoryPoolId::Test );
}

#if BLUE_MEMORY_ENABLE_POOL_ACCOUNTING
static void BlueMemoryContract_TypedAllocationUpdatesPoolStats( )
{
  Blue::MemoryPoolStats before = { };
  ASSERT_TRUE( Blue::CaptureMemoryPoolStats( Blue::MemoryPoolId::Test, before ) );

  ContractTestObject* object = Blue::BlueNew< ContractTestObject >( );
  ASSERT_TRUE( object != nullptr );

  Blue::MemoryPoolStats afterNew = { };
  ASSERT_TRUE( Blue::CaptureMemoryPoolStats( Blue::MemoryPoolId::Test, afterNew ) );

  ASSERT_TRUE( afterNew.CurrentBytes == before.CurrentBytes + sizeof( ContractTestObject ) );
  ASSERT_TRUE( afterNew.TotalAllocatedBytes == before.TotalAllocatedBytes + sizeof( ContractTestObject ) );
  ASSERT_TRUE( afterNew.TotalFreedBytes == before.TotalFreedBytes );
  ASSERT_TRUE( afterNew.AllocationCount == before.AllocationCount + 1 );
  ASSERT_TRUE( afterNew.FreeCount == before.FreeCount );
  ASSERT_TRUE( afterNew.FailedAllocationCount == before.FailedAllocationCount );
  ASSERT_TRUE( afterNew.BudgetExceededCount == before.BudgetExceededCount );
  ASSERT_TRUE( afterNew.PeakBytes >= afterNew.CurrentBytes );

  Blue::BlueDelete< ContractTestObject >( object );
  object = nullptr;

  Blue::MemoryPoolStats afterDelete = { };
  ASSERT_TRUE( Blue::CaptureMemoryPoolStats( Blue::MemoryPoolId::Test, afterDelete ) );

  ASSERT_TRUE( afterDelete.CurrentBytes == before.CurrentBytes );
  ASSERT_TRUE( afterDelete.TotalAllocatedBytes == before.TotalAllocatedBytes + sizeof( ContractTestObject ) );
  ASSERT_TRUE( afterDelete.TotalFreedBytes == before.TotalFreedBytes + sizeof( ContractTestObject ) );
  ASSERT_TRUE( afterDelete.AllocationCount == before.AllocationCount + 1 );
  ASSERT_TRUE( afterDelete.FreeCount == before.FreeCount + 1 );
  ASSERT_TRUE( afterDelete.FailedAllocationCount == before.FailedAllocationCount );
  ASSERT_TRUE( afterDelete.BudgetExceededCount == before.BudgetExceededCount );
  ASSERT_TRUE( afterDelete.PeakBytes >= afterNew.CurrentBytes );
}

static void BlueMemoryContract_RuntimeAllocationUpdatesSelectedPoolStats( )
{
  constexpr Blue::Size byteSize = 128;
  constexpr Blue::Size alignment = 16;
  constexpr Blue::MemoryPoolId pool = Blue::MemoryPoolId::Test;
  constexpr Blue::AllocationTag tag = Blue::AllocationTag::Test;

  Blue::MemoryPoolStats before = { };
  ASSERT_TRUE( Blue::CaptureMemoryPoolStats( pool, before ) );

  Blue::AllocationRequest request = BLUE_POOL_ALLOCATION_REQUEST( byteSize, alignment, tag, pool );

  void* pointer = Blue::BlueTryAllocate( request );
  ASSERT_TRUE( pointer != nullptr );

  Blue::MemoryPoolStats afterAllocate = { };
  ASSERT_TRUE( Blue::CaptureMemoryPoolStats( pool, afterAllocate ) );

  ASSERT_TRUE( afterAllocate.CurrentBytes == before.CurrentBytes + byteSize );
  ASSERT_TRUE( afterAllocate.TotalAllocatedBytes == before.TotalAllocatedBytes + byteSize );
  ASSERT_TRUE( afterAllocate.TotalFreedBytes == before.TotalFreedBytes );
  ASSERT_TRUE( afterAllocate.AllocationCount == before.AllocationCount + 1 );
  ASSERT_TRUE( afterAllocate.FreeCount == before.FreeCount );
  ASSERT_TRUE( afterAllocate.FailedAllocationCount == before.FailedAllocationCount );
  ASSERT_TRUE( afterAllocate.BudgetExceededCount == before.BudgetExceededCount );
  ASSERT_TRUE( afterAllocate.PeakBytes >= afterAllocate.CurrentBytes );

  Blue::BlueFree( Blue::AllocationFreeRequest{ pointer, byteSize, alignment, pool, tag } );
  pointer = nullptr;

  Blue::MemoryPoolStats afterFree = { };
  ASSERT_TRUE( Blue::CaptureMemoryPoolStats( pool, afterFree ) );

  ASSERT_TRUE( afterFree.CurrentBytes == before.CurrentBytes );
  ASSERT_TRUE( afterFree.TotalAllocatedBytes == before.TotalAllocatedBytes + byteSize );
  ASSERT_TRUE( afterFree.TotalFreedBytes == before.TotalFreedBytes + byteSize );
  ASSERT_TRUE( afterFree.AllocationCount == before.AllocationCount + 1 );
  ASSERT_TRUE( afterFree.FreeCount == before.FreeCount + 1 );
  ASSERT_TRUE( afterFree.FailedAllocationCount == before.FailedAllocationCount );
  ASSERT_TRUE( afterFree.BudgetExceededCount == before.BudgetExceededCount );
  ASSERT_TRUE( afterFree.PeakBytes >= afterAllocate.CurrentBytes );
}
#endif

static void BlueMemoryContract_InvalidSizeFails( )
{
  constexpr Blue::Size byteSize = 0;
  constexpr Blue::Size alignment = 16;
  constexpr Blue::MemoryPoolId pool = Blue::MemoryPoolId::Test;
  constexpr Blue::AllocationTag tag = Blue::AllocationTag::Test;

  Blue::MemoryPoolStats before = { };
  ASSERT_TRUE( Blue::CaptureMemoryPoolStats( pool, before ) );

  Blue::AllocationRequest request = BLUE_POOL_ALLOCATION_REQUEST( byteSize, alignment, tag, pool );

  void* pointer = Blue::BlueTryAllocate( request );
  ASSERT_TRUE( pointer == nullptr );

  Blue::MemoryPoolStats after = { };
  ASSERT_TRUE( Blue::CaptureMemoryPoolStats( pool, after ) );

  ExpectPoolStatsEqual( after, before );
}

static void BlueMemoryContract_InvalidAlignmentFails( )
{
  constexpr Blue::Size byteSize = 64;
  constexpr Blue::Size alignment = 3;
  constexpr Blue::MemoryPoolId pool = Blue::MemoryPoolId::Test;
  constexpr Blue::AllocationTag tag = Blue::AllocationTag::Test;

  Blue::MemoryPoolStats before = { };
  ASSERT_TRUE( Blue::CaptureMemoryPoolStats( pool, before ) );

  Blue::AllocationRequest request = BLUE_POOL_ALLOCATION_REQUEST( byteSize, alignment, tag, pool );

  void* pointer = Blue::BlueTryAllocate( request );
  ASSERT_TRUE( pointer == nullptr );

  Blue::MemoryPoolStats after = { };
  ASSERT_TRUE( Blue::CaptureMemoryPoolStats( pool, after ) );

  ExpectPoolStatsEqual( after, before );
}

static void BlueMemoryContract_InvalidPoolFails( )
{
  constexpr Blue::Size byteSize = 64;
  constexpr Blue::Size alignment = 16;
  constexpr Blue::AllocationTag tag = Blue::AllocationTag::Test;
  constexpr Blue::MemoryPoolId invalidPool = static_cast< Blue::MemoryPoolId >( 0xFF );

  AllPoolStats before = { };
  CaptureAllPoolStats( before );

  Blue::AllocationRequest request = BLUE_POOL_ALLOCATION_REQUEST( byteSize, alignment, tag, invalidPool );

  void* pointer = Blue::BlueTryAllocate( request );
  ASSERT_TRUE( pointer == nullptr );

  AllPoolStats after = { };
  CaptureAllPoolStats( after );

  ExpectAllPoolStatsEqual( after, before );
}

TEST( BlueMemoryContractTests, RunsSuccessfully )
{
  Blue::MemorySystemDesc desc = { };
  ASSERT_TRUE( Blue::InitializeMemorySystem( desc ).Succeeded( ) );

  BlueMemoryContract_PoolStatsAreZeroAfterInitialization( );

#if BLUE_MEMORY_ENABLE_POOL_ACCOUNTING
  BlueMemoryContract_TypedAllocationUpdatesPoolStats( );

  BlueMemoryContract_RuntimeAllocationUpdatesSelectedPoolStats( );
#endif

  BlueMemoryContract_InvalidSizeFails( );

  BlueMemoryContract_InvalidAlignmentFails( );

  BlueMemoryContract_InvalidPoolFails( );

  Blue::ShutdownMemorySystem( );
}

#include <Blue/Memory/BlueNew.h>
#include <Blue/Memory/Invoker/RuntimeAllocationInvoker.h>
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
  BLUE_TEST_EXPECT( actual.CurrentBytes == expected.CurrentBytes );
  BLUE_TEST_EXPECT( actual.PeakBytes == expected.PeakBytes );
  BLUE_TEST_EXPECT( actual.TotalAllocatedBytes == expected.TotalAllocatedBytes );
  BLUE_TEST_EXPECT( actual.TotalFreedBytes == expected.TotalFreedBytes );
  BLUE_TEST_EXPECT( actual.AllocationCount == expected.AllocationCount );
  BLUE_TEST_EXPECT( actual.FreeCount == expected.FreeCount );
  BLUE_TEST_EXPECT( actual.FailedAllocationCount == expected.FailedAllocationCount );
  BLUE_TEST_EXPECT( actual.BudgetExceededCount == expected.BudgetExceededCount );
}

static void CheckPoolStatsAreZero( Blue::MemoryPoolId poolId )
{
  Blue::MemoryPoolStats poolStats = { };

  BLUE_TEST_EXPECT( Blue::CaptureMemoryPoolStats( poolId, poolStats ) );

  BLUE_TEST_EXPECT( poolStats.CurrentBytes == 0 );
  BLUE_TEST_EXPECT( poolStats.PeakBytes == 0 );
  BLUE_TEST_EXPECT( poolStats.TotalAllocatedBytes == 0 );
  BLUE_TEST_EXPECT( poolStats.TotalFreedBytes == 0 );
  BLUE_TEST_EXPECT( poolStats.AllocationCount == 0 );
  BLUE_TEST_EXPECT( poolStats.FreeCount == 0 );
  BLUE_TEST_EXPECT( poolStats.FailedAllocationCount == 0 );
  BLUE_TEST_EXPECT( poolStats.BudgetExceededCount == 0 );
}

static void CaptureAllPoolStats( AllPoolStats& stats )
{
  BLUE_TEST_EXPECT( Blue::CaptureMemoryPoolStats( Blue::MemoryPoolId::System, stats.System ) );
  BLUE_TEST_EXPECT( Blue::CaptureMemoryPoolStats( Blue::MemoryPoolId::Renderer, stats.Renderer ) );
  BLUE_TEST_EXPECT( Blue::CaptureMemoryPoolStats( Blue::MemoryPoolId::Resources, stats.Resources ) );
  BLUE_TEST_EXPECT( Blue::CaptureMemoryPoolStats( Blue::MemoryPoolId::JobSystem, stats.JobSystem ) );
  BLUE_TEST_EXPECT( Blue::CaptureMemoryPoolStats( Blue::MemoryPoolId::Temporary, stats.Temporary ) );
  BLUE_TEST_EXPECT( Blue::CaptureMemoryPoolStats( Blue::MemoryPoolId::Test, stats.Test ) );
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

static void BlueMemoryContract_TypedAllocationUpdatesPoolStats( )
{
  Blue::MemoryPoolStats before = { };
  BLUE_TEST_EXPECT( Blue::CaptureMemoryPoolStats( Blue::MemoryPoolId::Test, before ) );

  ContractTestObject* object = Blue::BlueNew< ContractTestObject >( );
  BLUE_TEST_EXPECT( object != nullptr );

  Blue::MemoryPoolStats afterNew = { };
  BLUE_TEST_EXPECT( Blue::CaptureMemoryPoolStats( Blue::MemoryPoolId::Test, afterNew ) );

  BLUE_TEST_EXPECT( afterNew.CurrentBytes == before.CurrentBytes + sizeof( ContractTestObject ) );
  BLUE_TEST_EXPECT( afterNew.TotalAllocatedBytes == before.TotalAllocatedBytes + sizeof( ContractTestObject ) );
  BLUE_TEST_EXPECT( afterNew.TotalFreedBytes == before.TotalFreedBytes );
  BLUE_TEST_EXPECT( afterNew.AllocationCount == before.AllocationCount + 1 );
  BLUE_TEST_EXPECT( afterNew.FreeCount == before.FreeCount );
  BLUE_TEST_EXPECT( afterNew.FailedAllocationCount == before.FailedAllocationCount );
  BLUE_TEST_EXPECT( afterNew.BudgetExceededCount == before.BudgetExceededCount );
  BLUE_TEST_EXPECT( afterNew.PeakBytes >= afterNew.CurrentBytes );

  Blue::BlueDelete< ContractTestObject >( object );
  object = nullptr;

  Blue::MemoryPoolStats afterDelete = { };
  BLUE_TEST_EXPECT( Blue::CaptureMemoryPoolStats( Blue::MemoryPoolId::Test, afterDelete ) );

  BLUE_TEST_EXPECT( afterDelete.CurrentBytes == before.CurrentBytes );
  BLUE_TEST_EXPECT( afterDelete.TotalAllocatedBytes == before.TotalAllocatedBytes + sizeof( ContractTestObject ) );
  BLUE_TEST_EXPECT( afterDelete.TotalFreedBytes == before.TotalFreedBytes + sizeof( ContractTestObject ) );
  BLUE_TEST_EXPECT( afterDelete.AllocationCount == before.AllocationCount + 1 );
  BLUE_TEST_EXPECT( afterDelete.FreeCount == before.FreeCount + 1 );
  BLUE_TEST_EXPECT( afterDelete.FailedAllocationCount == before.FailedAllocationCount );
  BLUE_TEST_EXPECT( afterDelete.BudgetExceededCount == before.BudgetExceededCount );
  BLUE_TEST_EXPECT( afterDelete.PeakBytes >= afterNew.CurrentBytes );
}

static void BlueMemoryContract_RuntimeAllocationUpdatesSelectedPoolStats( )
{
  constexpr Blue::Size byteSize = 128;
  constexpr Blue::Size alignment = 16;
  constexpr Blue::MemoryPoolId pool = Blue::MemoryPoolId::Test;
  constexpr Blue::AllocationTag tag = Blue::AllocationTag::Test;

  Blue::MemoryPoolStats before = { };
  BLUE_TEST_EXPECT( Blue::CaptureMemoryPoolStats( pool, before ) );

  Blue::AllocationRequest request = BLUE_POOL_ALLOCATION_REQUEST( byteSize, alignment, tag, pool );

  void* pointer = Blue::BlueTryAllocate( request );
  BLUE_TEST_EXPECT( pointer != nullptr );

  Blue::MemoryPoolStats afterAllocate = { };
  BLUE_TEST_EXPECT( Blue::CaptureMemoryPoolStats( pool, afterAllocate ) );

  BLUE_TEST_EXPECT( afterAllocate.CurrentBytes == before.CurrentBytes + byteSize );
  BLUE_TEST_EXPECT( afterAllocate.TotalAllocatedBytes == before.TotalAllocatedBytes + byteSize );
  BLUE_TEST_EXPECT( afterAllocate.TotalFreedBytes == before.TotalFreedBytes );
  BLUE_TEST_EXPECT( afterAllocate.AllocationCount == before.AllocationCount + 1 );
  BLUE_TEST_EXPECT( afterAllocate.FreeCount == before.FreeCount );
  BLUE_TEST_EXPECT( afterAllocate.FailedAllocationCount == before.FailedAllocationCount );
  BLUE_TEST_EXPECT( afterAllocate.BudgetExceededCount == before.BudgetExceededCount );
  BLUE_TEST_EXPECT( afterAllocate.PeakBytes >= afterAllocate.CurrentBytes );

  Blue::BlueFree( Blue::AllocationFreeRequest{ pointer, byteSize, alignment, pool, tag } );
  pointer = nullptr;

  Blue::MemoryPoolStats afterFree = { };
  BLUE_TEST_EXPECT( Blue::CaptureMemoryPoolStats( pool, afterFree ) );

  BLUE_TEST_EXPECT( afterFree.CurrentBytes == before.CurrentBytes );
  BLUE_TEST_EXPECT( afterFree.TotalAllocatedBytes == before.TotalAllocatedBytes + byteSize );
  BLUE_TEST_EXPECT( afterFree.TotalFreedBytes == before.TotalFreedBytes + byteSize );
  BLUE_TEST_EXPECT( afterFree.AllocationCount == before.AllocationCount + 1 );
  BLUE_TEST_EXPECT( afterFree.FreeCount == before.FreeCount + 1 );
  BLUE_TEST_EXPECT( afterFree.FailedAllocationCount == before.FailedAllocationCount );
  BLUE_TEST_EXPECT( afterFree.BudgetExceededCount == before.BudgetExceededCount );
  BLUE_TEST_EXPECT( afterFree.PeakBytes >= afterAllocate.CurrentBytes );
}

static void BlueMemoryContract_InvalidSizeFails( )
{
  constexpr Blue::Size byteSize = 0;
  constexpr Blue::Size alignment = 16;
  constexpr Blue::MemoryPoolId pool = Blue::MemoryPoolId::Test;
  constexpr Blue::AllocationTag tag = Blue::AllocationTag::Test;

  Blue::MemoryPoolStats before = { };
  BLUE_TEST_EXPECT( Blue::CaptureMemoryPoolStats( pool, before ) );

  Blue::AllocationRequest request = BLUE_POOL_ALLOCATION_REQUEST( byteSize, alignment, tag, pool );

  void* pointer = Blue::BlueTryAllocate( request );
  BLUE_TEST_EXPECT( pointer == nullptr );

  Blue::MemoryPoolStats after = { };
  BLUE_TEST_EXPECT( Blue::CaptureMemoryPoolStats( pool, after ) );

  ExpectPoolStatsEqual( after, before );
}

static void BlueMemoryContract_InvalidAlignmentFails( )
{
  constexpr Blue::Size byteSize = 64;
  constexpr Blue::Size alignment = 3;
  constexpr Blue::MemoryPoolId pool = Blue::MemoryPoolId::Test;
  constexpr Blue::AllocationTag tag = Blue::AllocationTag::Test;

  Blue::MemoryPoolStats before = { };
  BLUE_TEST_EXPECT( Blue::CaptureMemoryPoolStats( pool, before ) );

  Blue::AllocationRequest request = BLUE_POOL_ALLOCATION_REQUEST( byteSize, alignment, tag, pool );

  void* pointer = Blue::BlueTryAllocate( request );
  BLUE_TEST_EXPECT( pointer == nullptr );

  Blue::MemoryPoolStats after = { };
  BLUE_TEST_EXPECT( Blue::CaptureMemoryPoolStats( pool, after ) );

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
  BLUE_TEST_EXPECT( pointer == nullptr );

  AllPoolStats after = { };
  CaptureAllPoolStats( after );

  ExpectAllPoolStatsEqual( after, before );
}

int main( )
{
  Blue::MemorySystemDesc desc = { };
  BLUE_TEST_EXPECT( Blue::InitializeMemorySystem( desc ).Succeeded( ) );

  BlueMemoryContract_PoolStatsAreZeroAfterInitialization( );

  BlueMemoryContract_TypedAllocationUpdatesPoolStats( );

  BlueMemoryContract_RuntimeAllocationUpdatesSelectedPoolStats( );

  BlueMemoryContract_InvalidSizeFails( );

  BlueMemoryContract_InvalidAlignmentFails( );

  BlueMemoryContract_InvalidPoolFails( );

  Blue::ShutdownMemorySystem( );

  printf( "BlueMemory contract tests passed.\n" );
  return 0;
}

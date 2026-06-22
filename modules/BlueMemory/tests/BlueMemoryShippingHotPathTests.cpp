// Copyright (c) Khaled Shawki. All rights reserved.

#include <Blue/Memory/Allocator.h>
#include <Blue/Memory/Config/BlueMemoryConfig.h>
#include <Blue/Memory/Invoker/RuntimeAllocationInvoker.h>
#include <Blue/Memory/MemorySystem.h>
#include <Blue/Memory/Metrics/MemoryThreadContext.h>
#include <Blue/Memory/Pool/MemoryPoolRegistry.h>

#include <gtest/gtest.h>

namespace
{
#if !BLUE_MEMORY_ENABLE_POOL_ACCOUNTING || !BLUE_MEMORY_ENABLE_METRICS

constexpr Blue::MemoryPoolId TestPool = Blue::MemoryPoolId::Test;
constexpr Blue::Size TestAllocationSize = 64;
constexpr Blue::Size TestAllocationAlignment = 16;

struct MemorySystemTestScope
{
  MemorySystemTestScope( )
  {
    Blue::MemorySystemDesc desc = { };
    Initialized = Blue::InitializeMemorySystem( desc ).Succeeded( );
  }

  ~MemorySystemTestScope( )
  {
    if ( Initialized )
    {
      Blue::ShutdownMemorySystem( );
    }
  }

  bool Initialized = false;
};

Blue::AllocationRequest MakeRuntimeRequest( )
{
  return BLUE_POOL_ALLOCATION_REQUEST( TestAllocationSize,
                                       TestAllocationAlignment,
                                       Blue::AllocationTag::Test,
                                       TestPool );
}

Blue::AllocationFreeRequest MakeFreeRequest( void* pointer )
{
  return Blue::AllocationFreeRequest{ pointer,
                                      TestAllocationSize,
                                      TestAllocationAlignment,
                                      TestPool,
                                      Blue::AllocationTag::Test };
}

#endif

#if !BLUE_MEMORY_ENABLE_METRICS

Blue::Uint64 SumThreadAllocationCounts( )
{
  Blue::MemoryThreadContext contexts[ Blue::BlueMemoryMaxTrackedThreads ] = { };
  const Blue::Size count = Blue::CaptureMemoryThreadContexts( contexts, Blue::BlueMemoryMaxTrackedThreads );

  Blue::Uint64 total = 0;
  for ( Blue::Size contextIndex = 0; contextIndex < count; ++contextIndex )
  {
    for ( Blue::Size poolIndex = 0; poolIndex < Blue::MemoryPoolCount; ++poolIndex )
    {
      total += contexts[ contextIndex ].PoolMetrics[ poolIndex ].AllocationCount;
      total += contexts[ contextIndex ].PoolMetrics[ poolIndex ].FreeCount;
      total += contexts[ contextIndex ].PoolMetrics[ poolIndex ].FailedAllocationCount;
      total += contexts[ contextIndex ].PoolMetrics[ poolIndex ].CurrentBytes;
      total += contexts[ contextIndex ].PoolMetrics[ poolIndex ].PeakBytes;
    }
  }

  return total;
}

#endif
} // namespace
TEST( BlueMemoryShippingHotPathTests, RuntimeAllocatorDoesNotMutatePoolStatsWhenPoolAccountingIsCompiledOut )
{
#if BLUE_MEMORY_ENABLE_POOL_ACCOUNTING
  GTEST_SKIP( ) << "Pool accounting is compiled in for this build.";
#else
  MemorySystemTestScope memory;
  ASSERT_TRUE( memory.Initialized );

  Blue::MemoryPoolStats before = { };
  ASSERT_TRUE( Blue::CaptureMemoryPoolStats( TestPool, before ) );

  void* pointer = Blue::BlueTryAllocate( MakeRuntimeRequest( ) );
  ASSERT_NE( pointer, nullptr );

  Blue::BlueFree( MakeFreeRequest( pointer ) );

  Blue::MemoryPoolStats after = { };
  ASSERT_TRUE( Blue::CaptureMemoryPoolStats( TestPool, after ) );

  EXPECT_EQ( after.CurrentBytes, before.CurrentBytes );
  EXPECT_EQ( after.TotalAllocatedBytes, before.TotalAllocatedBytes );
  EXPECT_EQ( after.TotalFreedBytes, before.TotalFreedBytes );
  EXPECT_EQ( after.AllocationCount, before.AllocationCount );
  EXPECT_EQ( after.FreeCount, before.FreeCount );
  EXPECT_EQ( after.FailedAllocationCount, before.FailedAllocationCount );
#endif
}

TEST( BlueMemoryShippingHotPathTests, HeapAllocatorDoesNotMutatePoolStatsWhenPoolAccountingIsCompiledOut )
{
#if BLUE_MEMORY_ENABLE_POOL_ACCOUNTING
  GTEST_SKIP( ) << "Pool accounting is compiled in for this build.";
#else
  MemorySystemTestScope memory;
  ASSERT_TRUE( memory.Initialized );

  Blue::MemoryPoolStats before = { };
  ASSERT_TRUE( Blue::CaptureMemoryPoolStats( TestPool, before ) );

  Blue::Allocator allocator = Blue::GetDefaultAllocator( );
  Blue::AllocationResult allocation = Blue::Allocate( allocator, MakeRuntimeRequest( ) );
  ASSERT_NE( allocation.Pointer, nullptr );

  Blue::Free( allocator, MakeFreeRequest( allocation.Pointer ) );

  Blue::MemoryPoolStats after = { };
  ASSERT_TRUE( Blue::CaptureMemoryPoolStats( TestPool, after ) );

  EXPECT_EQ( after.CurrentBytes, before.CurrentBytes );
  EXPECT_EQ( after.TotalAllocatedBytes, before.TotalAllocatedBytes );
  EXPECT_EQ( after.TotalFreedBytes, before.TotalFreedBytes );
  EXPECT_EQ( after.AllocationCount, before.AllocationCount );
  EXPECT_EQ( after.FreeCount, before.FreeCount );
  EXPECT_EQ( after.FailedAllocationCount, before.FailedAllocationCount );
#endif
}

TEST( BlueMemoryShippingHotPathTests, RuntimeAllocatorDoesNotMutateThreadMetricsWhenMetricsAreCompiledOut )
{
#if BLUE_MEMORY_ENABLE_METRICS
  GTEST_SKIP( ) << "Metrics are compiled in for this build.";
#else
  MemorySystemTestScope memory;
  ASSERT_TRUE( memory.Initialized );

  const Blue::Uint64 before = SumThreadAllocationCounts( );

  void* pointer = Blue::BlueTryAllocate( MakeRuntimeRequest( ) );
  ASSERT_NE( pointer, nullptr );

  Blue::BlueFree( MakeFreeRequest( pointer ) );

  const Blue::Uint64 after = SumThreadAllocationCounts( );

  EXPECT_EQ( after, before );
#endif
}

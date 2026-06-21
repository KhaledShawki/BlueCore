// Copyright (c) Khaled Shawki. All rights reserved.

#include <Blue/Memory/Allocator.h>
#include <Blue/Memory/Config/BlueMemoryConfig.h>
#include <Blue/Memory/Invoker/RuntimeAllocationInvoker.h>
#include <Blue/Memory/MemorySystem.h>
#include <Blue/Memory/MemoryUnits.h>
#include <Blue/Memory/Pool/MemoryPoolRegistry.h>

#include <gtest/gtest.h>

namespace
{
constexpr Blue::MemoryPoolId BudgetedPool = Blue::MemoryPoolId::Test;
constexpr Blue::Size TestPoolBudgetBytes = BLUE_MB( 16 );
constexpr Blue::Size OverBudgetAllocationBytes = TestPoolBudgetBytes + 16;

struct MemorySystemTestScope
{
  explicit MemorySystemTestScope( const Blue::MemorySystemDesc& desc )
  {
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

Blue::MemorySystemDesc MakePoolBudgetDesc( bool enableRuntimeBudgets )
{
  Blue::MemorySystemDesc desc = { };
  desc.Settings.EnableRuntimePoolBudgets = enableRuntimeBudgets;
  return desc;
}

Blue::AllocationRequest MakeOverBudgetRequest( )
{
  return BLUE_POOL_ALLOCATION_REQUEST( OverBudgetAllocationBytes, 16, Blue::AllocationTag::Test, BudgetedPool );
}

void FreeOverBudgetAllocation( void* pointer )
{
  Blue::BlueFree(
    Blue::AllocationFreeRequest{ pointer, OverBudgetAllocationBytes, 16, BudgetedPool, Blue::AllocationTag::Test } );
}
} // namespace

TEST( BlueMemoryPoolBudgetSettingsTests, RuntimeBudgetSettingResolvesByBuildProfile )
{
  Blue::MemorySystemDesc desc = MakePoolBudgetDesc( true );

  MemorySystemTestScope memory( desc );
  ASSERT_TRUE( memory.Initialized );

#if BLUE_MEMORY_ENABLE_POOL_ACCOUNTING
  EXPECT_TRUE( Blue::GetMemorySettings( ).EnableRuntimePoolBudgets );
  EXPECT_TRUE( Blue::GetMemoryPoolRegistry( ).IsBudgetEnforcementEnabled( ) );
#else
  EXPECT_FALSE( Blue::GetMemorySettings( ).EnableRuntimePoolBudgets );
  EXPECT_FALSE( Blue::GetMemoryPoolRegistry( ).IsBudgetEnforcementEnabled( ) );
#endif
}

TEST( BlueMemoryPoolBudgetSettingsTests, RuntimeAllocatorRejectsOverBudgetAllocationWhenBudgetsAreEnabled )
{
#if BLUE_MEMORY_ENABLE_POOL_ACCOUNTING
  Blue::MemorySystemDesc desc = MakePoolBudgetDesc( true );

  MemorySystemTestScope memory( desc );
  ASSERT_TRUE( memory.Initialized );
  ASSERT_TRUE( Blue::GetMemoryPoolRegistry( ).IsBudgetEnforcementEnabled( ) );

  void* pointer = Blue::BlueTryAllocate( MakeOverBudgetRequest( ) );

  EXPECT_EQ( pointer, nullptr );

  Blue::MemoryPoolStats stats = { };
  ASSERT_TRUE( Blue::CaptureMemoryPoolStats( BudgetedPool, stats ) );
  EXPECT_GE( stats.BudgetExceededCount, 1u );
#else
  GTEST_SKIP( ) << "Pool accounting is compiled out for this build.";
#endif
}

TEST( BlueMemoryPoolBudgetSettingsTests, RuntimeAllocatorAllowsOverBudgetAllocationWhenBudgetsAreDisabled )
{
  Blue::MemorySystemDesc desc = MakePoolBudgetDesc( false );

  MemorySystemTestScope memory( desc );
  ASSERT_TRUE( memory.Initialized );
  EXPECT_FALSE( Blue::GetMemoryPoolRegistry( ).IsBudgetEnforcementEnabled( ) );

  void* pointer = Blue::BlueTryAllocate( MakeOverBudgetRequest( ) );

  ASSERT_NE( pointer, nullptr );

#if BLUE_MEMORY_ENABLE_POOL_ACCOUNTING
  Blue::MemoryPoolStats during = { };
  ASSERT_TRUE( Blue::CaptureMemoryPoolStats( BudgetedPool, during ) );
  EXPECT_GE( during.CurrentBytes, OverBudgetAllocationBytes );
  EXPECT_EQ( during.BudgetExceededCount, 0u );
#endif

  FreeOverBudgetAllocation( pointer );

#if BLUE_MEMORY_ENABLE_POOL_ACCOUNTING
  Blue::MemoryPoolStats after = { };
  ASSERT_TRUE( Blue::CaptureMemoryPoolStats( BudgetedPool, after ) );
  EXPECT_EQ( after.CurrentBytes, 0u );
#endif
}

TEST( BlueMemoryPoolBudgetSettingsTests, HeapAllocatorRejectsOverBudgetAllocationWhenBudgetsAreEnabled )
{
#if BLUE_MEMORY_ENABLE_POOL_ACCOUNTING
  Blue::MemorySystemDesc desc = MakePoolBudgetDesc( true );

  MemorySystemTestScope memory( desc );
  ASSERT_TRUE( memory.Initialized );
  ASSERT_TRUE( Blue::GetMemoryPoolRegistry( ).IsBudgetEnforcementEnabled( ) );

  Blue::Allocator allocator = Blue::GetDefaultAllocator( );

  Blue::AllocationResult allocation = Blue::Allocate( allocator, MakeOverBudgetRequest( ) );

  EXPECT_EQ( allocation.Pointer, nullptr );
  EXPECT_EQ( allocation.ByteSize, 0u );

  Blue::MemoryPoolStats stats = { };
  ASSERT_TRUE( Blue::CaptureMemoryPoolStats( BudgetedPool, stats ) );
  EXPECT_GE( stats.BudgetExceededCount, 1u );
#else
  GTEST_SKIP( ) << "Pool accounting is compiled out for this build.";
#endif
}

TEST( BlueMemoryPoolBudgetSettingsTests, HeapAllocatorAllowsOverBudgetAllocationWhenBudgetsAreDisabled )
{
  Blue::MemorySystemDesc desc = MakePoolBudgetDesc( false );

  MemorySystemTestScope memory( desc );
  ASSERT_TRUE( memory.Initialized );
  EXPECT_FALSE( Blue::GetMemoryPoolRegistry( ).IsBudgetEnforcementEnabled( ) );

  Blue::Allocator allocator = Blue::GetDefaultAllocator( );

  Blue::AllocationResult allocation = Blue::Allocate( allocator, MakeOverBudgetRequest( ) );

  ASSERT_NE( allocation.Pointer, nullptr );
  EXPECT_EQ( allocation.ByteSize, OverBudgetAllocationBytes );

#if BLUE_MEMORY_ENABLE_POOL_ACCOUNTING
  Blue::MemoryPoolStats during = { };
  ASSERT_TRUE( Blue::CaptureMemoryPoolStats( BudgetedPool, during ) );
  EXPECT_GE( during.CurrentBytes, OverBudgetAllocationBytes );
  EXPECT_EQ( during.BudgetExceededCount, 0u );
#endif

  Blue::Free( allocator,
              Blue::AllocationFreeRequest{ allocation.Pointer,
                                           allocation.ByteSize,
                                           16,
                                           BudgetedPool,
                                           Blue::AllocationTag::Test } );

#if BLUE_MEMORY_ENABLE_POOL_ACCOUNTING
  Blue::MemoryPoolStats after = { };
  ASSERT_TRUE( Blue::CaptureMemoryPoolStats( BudgetedPool, after ) );
  EXPECT_EQ( after.CurrentBytes, 0u );
#endif
}

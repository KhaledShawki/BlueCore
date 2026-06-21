// Copyright (c) Khaled Shawki. All rights reserved.

#include <Blue/Memory/Allocator.h>
#include <Blue/Memory/Config/BlueMemoryConfig.h>
#include <Blue/Memory/Invoker/RuntimeAllocationInvoker.h>
#include <Blue/Memory/MemorySystem.h>
#include <Blue/Memory/Oom/OomReporter.h>
#include <Blue/System/Types.h>

#include <gtest/gtest.h>

namespace
{
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

struct OomCallbackContext
{
  Blue::Uint32 Count = 0;
  Blue::AllocationFailureReason Reason = Blue::AllocationFailureReason::None;
  Blue::Size RequestedSize = 0;
};

static OomCallbackContext g_OomCallbackContext = { };

void TestOomCallback( Blue::AllocationFailureReason reason, Blue::Size size ) noexcept
{
  ++g_OomCallbackContext.Count;
  g_OomCallbackContext.Reason = reason;
  g_OomCallbackContext.RequestedSize = size;
}

Blue::MemorySystemDesc MakeRuntimeSettingsDesc( )
{
  Blue::MemorySystemDesc desc = { };
  desc.Settings.OutOfMemoryPolicy = Blue::OomPolicy::ReportAndReturnNull;
  desc.Settings.EnableRuntimeLeakReportOnShutdown = true;
  return desc;
}
} // namespace

TEST( BlueMemoryRuntimeSettingsTests, StoresResolvedRuntimeSettings )
{
  Blue::MemorySystemDesc desc = MakeRuntimeSettingsDesc( );
  desc.Settings.EnableRuntimeMetricsFlush = false;

  MemorySystemTestScope memory( desc );
  ASSERT_TRUE( memory.Initialized );

  const Blue::BlueMemorySettings& settings = Blue::GetMemorySettings( );

  EXPECT_EQ( settings.OutOfMemoryPolicy, Blue::OomPolicy::ReportAndReturnNull );
  EXPECT_EQ( settings.EnableRuntimeMetricsFlush, false );

#if BLUE_MEMORY_ENABLE_LEAK_REPORTS
  EXPECT_EQ( settings.EnableRuntimeLeakReportOnShutdown, true );
#else
  EXPECT_EQ( settings.EnableRuntimeLeakReportOnShutdown, false );
#endif
}

TEST( BlueMemoryRuntimeSettingsTests, ReturnNullPolicySuppressesOomReports )
{
#if BLUE_MEMORY_ENABLE_OOM_REPORTS
  Blue::OomReport reports[ 4 ] = { };

  Blue::MemorySystemDesc desc = MakeRuntimeSettingsDesc( );
  desc.OomReportBuffer = reports;
  desc.OomReportCapacity = 4;
  desc.Settings.OutOfMemoryPolicy = Blue::OomPolicy::ReturnNull;

  MemorySystemTestScope memory( desc );
  ASSERT_TRUE( memory.Initialized );

  Blue::AllocationRequest request =
    BLUE_POOL_ALLOCATION_REQUEST( 64, 3, Blue::AllocationTag::Test, Blue::MemoryPoolId::Test );

  void* pointer = Blue::BlueTryAllocate( request );
  EXPECT_EQ( pointer, nullptr );

  Blue::OomReport captured[ 4 ] = { };
  EXPECT_EQ( Blue::CaptureOomReports( captured, 4 ), 0 );
#else
  GTEST_SKIP( ) << "OOM reports are compiled out for this build.";
#endif
}

TEST( BlueMemoryRuntimeSettingsTests, ReportAndReturnNullPolicyRecordsOomReports )
{
#if BLUE_MEMORY_ENABLE_OOM_REPORTS
  Blue::OomReport reports[ 4 ] = { };

  Blue::MemorySystemDesc desc = MakeRuntimeSettingsDesc( );
  desc.OomReportBuffer = reports;
  desc.OomReportCapacity = 4;
  desc.Settings.OutOfMemoryPolicy = Blue::OomPolicy::ReportAndReturnNull;

  MemorySystemTestScope memory( desc );
  ASSERT_TRUE( memory.Initialized );

  Blue::AllocationRequest request =
    BLUE_POOL_ALLOCATION_REQUEST( 64, 3, Blue::AllocationTag::Test, Blue::MemoryPoolId::Test );

  void* pointer = Blue::BlueTryAllocate( request );
  EXPECT_EQ( pointer, nullptr );

  Blue::OomReport captured[ 4 ] = { };
  const Blue::Size count = Blue::CaptureOomReports( captured, 4 );

  ASSERT_EQ( count, 1 );
  EXPECT_EQ( captured[ 0 ].Reason, Blue::AllocationFailureReason::InvalidAlignment );
#else
  GTEST_SKIP( ) << "OOM reports are compiled out for this build.";
#endif
}

TEST( BlueMemoryRuntimeSettingsTests, CallbackPolicyInvokesLightweightCallback )
{
  g_OomCallbackContext = { };

  Blue::MemorySystemDesc desc = MakeRuntimeSettingsDesc( );
  desc.Settings.OutOfMemoryPolicy = Blue::OomPolicy::Callback;
  desc.Settings.OutOfMemoryCallback = &TestOomCallback;

  MemorySystemTestScope memory( desc );
  ASSERT_TRUE( memory.Initialized );

  Blue::AllocationRequest request =
    BLUE_POOL_ALLOCATION_REQUEST( 64, 3, Blue::AllocationTag::Test, Blue::MemoryPoolId::Test );

  void* pointer = Blue::BlueTryAllocate( request );
  EXPECT_EQ( pointer, nullptr );

  EXPECT_EQ( g_OomCallbackContext.Count, 1u );
  EXPECT_EQ( g_OomCallbackContext.Reason, Blue::AllocationFailureReason::InvalidAlignment );
  EXPECT_EQ( g_OomCallbackContext.RequestedSize, 64u );
}

TEST( BlueMemoryRuntimeSettingsTests, BlueAllocateCallbackPolicyReportsFailureOnce )
{
  g_OomCallbackContext = { };

  Blue::MemorySystemDesc desc = MakeRuntimeSettingsDesc( );
  desc.Settings.OutOfMemoryPolicy = Blue::OomPolicy::Callback;
  desc.Settings.OutOfMemoryCallback = &TestOomCallback;

  MemorySystemTestScope memory( desc );
  ASSERT_TRUE( memory.Initialized );

  Blue::AllocationRequest request =
    BLUE_POOL_ALLOCATION_REQUEST( 64, 3, Blue::AllocationTag::Test, Blue::MemoryPoolId::Test );

  void* pointer = Blue::BlueAllocate( request );
  EXPECT_EQ( pointer, nullptr );

  EXPECT_EQ( g_OomCallbackContext.Count, 1u );
  EXPECT_EQ( g_OomCallbackContext.Reason, Blue::AllocationFailureReason::InvalidAlignment );
  EXPECT_EQ( g_OomCallbackContext.RequestedSize, 64u );
}

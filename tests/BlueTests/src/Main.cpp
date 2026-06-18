#include <Blue/Container/DynamicArray.h>
#include <Blue/Container/SmidString.h>
#include <Blue/JobSystem/JobSystem.h>
#include <Blue/Memory/LinearAllocator.h>
#include <Blue/Memory/MemorySystem.h>
#include <Blue/System/Log/LogCategory.h>
#include <Blue/System/Log/LogMacros.h>
#include <Blue/System/Log/Logger.h>
#include <Blue/System/Types.h>

#include <cstdio>

#include "TestFramework.h"

BLUE_DEFINE_LOG_CATEGORY( LogTests, Blue::LogLevel::Info );

BLUE_TEST( TypeSizeTests )
{
  BLUE_EXPECT_EQ( sizeof( Blue::Uint8 ), 1u );
  BLUE_EXPECT_EQ( sizeof( Blue::Uint16 ), 2u );
  BLUE_EXPECT_EQ( sizeof( Blue::Uint32 ), 4u );
  BLUE_EXPECT_EQ( sizeof( Blue::Uint64 ), 8u );
  return true;
}

BLUE_TEST( LinearAllocatorTests )
{
  Blue::Byte buffer[ 1024 ] = { };
  Blue::LinearAllocator linear = { };
  Blue::InitializeLinearAllocator( linear, buffer, sizeof( buffer ), Blue::AllocationTag::Test );
  Blue::Allocator allocator = Blue::MakeLinearAllocator( linear );

  Blue::AllocationResult a = Blue::Allocate( allocator, BLUE_ALLOCATION_REQUEST( 64, 8, Blue::AllocationTag::Test ) );
  Blue::AllocationResult b = Blue::Allocate( allocator, BLUE_ALLOCATION_REQUEST( 128, 16, Blue::AllocationTag::Test ) );

  BLUE_EXPECT_TRUE( a.Pointer != nullptr );
  BLUE_EXPECT_TRUE( b.Pointer != nullptr );
  BLUE_EXPECT_TRUE( a.Pointer != b.Pointer );
  return true;
}

BLUE_TEST( SmidStringTests )
{
  Blue::Allocator allocator = Blue::GetDefaultAllocator( );
  Blue::SmidString small( "Blue", allocator );
  BLUE_EXPECT_TRUE( small.IsInline( ) );
  BLUE_EXPECT_EQ( small.GetSize( ), 4u );

  Blue::SmidString large( "This string is intentionally larger than the inline buffer", allocator );
  BLUE_EXPECT_TRUE( !large.IsInline( ) );
  BLUE_EXPECT_TRUE( large.GetSize( ) > Blue::SmidString::InlineCapacity );
  return true;
}

BLUE_TEST( DynamicArrayTests )
{
  Blue::DynamicArray< Blue::Uint32 > values( Blue::GetDefaultAllocator( ) );
  BLUE_EXPECT_TRUE( values.PushBack( 7 ) );
  BLUE_EXPECT_TRUE( values.PushBack( 9 ) );
  BLUE_EXPECT_EQ( values.GetSize( ), 2u );
  BLUE_EXPECT_EQ( values[ 0 ], 7u );
  BLUE_EXPECT_EQ( values[ 1 ], 9u );
  return true;
}

static void JobFunction( void* userData )
{
  Blue::Uint32* value = static_cast< Blue::Uint32* >( userData );
  *value += 1;
}

BLUE_TEST( JobSystemTests )
{
  static Blue::JobSystem jobs;
  BLUE_EXPECT_TRUE( jobs.Initialize( Blue::GetDefaultAllocator( ), 0 ) );
  Blue::Uint32 value = 0;
  BLUE_EXPECT_TRUE( jobs.Submit( { JobFunction, &value } ) );
  jobs.ExecutePendingJobsOnCurrentThread( );
  BLUE_EXPECT_EQ( value, 1u );
  jobs.Shutdown( );
  return true;
}

namespace
{
using TestFunction = bool ( * )( );

struct TestCase final
{
  const char* Name;
  TestFunction Function;
};

struct TestSummary final
{
  Blue::Uint32 Total = 0;
  Blue::Uint32 Passed = 0;
  Blue::Uint32 Failed = 0;
};

void PrintLine( )
{
  std::printf( "------------------------------------------------------------\n" );
}

void PrintHeader( )
{
  std::printf( "============================================================\n" );
  std::printf( "Blue Test Suite\n" );
  std::printf( "============================================================\n" );
}

bool RunTestCase( const TestCase& testCase, TestSummary& summary )
{
  ++summary.Total;

  std::printf( "[ RUN      ] %s\n", testCase.Name );

  const bool passed = testCase.Function( );
  if ( passed )
  {
    ++summary.Passed;
    std::printf( "[       OK ] %s\n", testCase.Name );
    return true;
  }

  ++summary.Failed;
  std::printf( "[  FAILED  ] %s\n", testCase.Name );
  return false;
}

void PrintSummary( const TestSummary& summary )
{
  PrintLine( );
  std::printf( "Blue Test Summary\n" );
  std::printf( "  Total : %u\n", summary.Total );
  std::printf( "  Passed: %u\n", summary.Passed );
  std::printf( "  Failed: %u\n", summary.Failed );
  PrintLine( );

  if ( summary.Failed == 0 )
  {
    std::printf( "All Blue tests passed.\n" );
  }
  else
  {
    std::printf( "Blue tests failed.\n" );
  }
}
} // namespace

int main( )
{
  Blue::RegisterLogSink( Blue::CreateConsoleLogSink( Blue::LogLevel::Info ) );

  Blue::MemorySystemDesc memoryDesc = { };
  memoryDesc.EnableMetrics = true;
  memoryDesc.EnableTracking = true;
  memoryDesc.EnableLeakDetection = true;
  memoryDesc.TrackingCapacity = 4096;

  Blue::Result memoryResult = Blue::InitializeMemorySystem( memoryDesc );
  if ( memoryResult.Failed( ) )
  {
    std::printf( "[  FAILED  ] InitializeMemorySystem\n" );
    return 1;
  }

  BLUE_LOG_INFO( LogTests, "Running Blue tests" );

  const TestCase tests[] = {
    {"TypeSizeTests",        &TypeSizeTests       },
    {"LinearAllocatorTests", &LinearAllocatorTests},
    {"SmidStringTests",      &SmidStringTests     },
    {"DynamicArrayTests",    &DynamicArrayTests   },
    {"JobSystemTests",       &JobSystemTests      },
  };

  PrintHeader( );

  TestSummary summary = { };
  for ( const TestCase& testCase : tests )
  {
    RunTestCase( testCase, summary );
  }

  Blue::ShutdownMemorySystem( );

  PrintSummary( summary );
  return summary.Failed == 0 ? 0 : 1;
}

#include <Blue/System/Alignment.h>
#include <Blue/System/Threading/Processor.h>
#include <Blue/System/Types.h>

#include <gtest/gtest.h>


TEST( BlueSystemProcessorTests, ProcessorInfoReportsValidRuntimeValues )
{
  const Blue::ProcessorArchitecture architecture = Blue::GetProcessorArchitecture( );
  const Blue::Char* architectureName = Blue::GetProcessorArchitectureName( architecture );

  ASSERT_NE( architectureName, nullptr );
  EXPECT_NE( architectureName[ 0 ], '\0' );

  const Blue::Uint32 logicalProcessorCount = Blue::GetLogicalProcessorCount( );
  EXPECT_GT( logicalProcessorCount, 0u );

  const Blue::Uint32 cacheLineSize = Blue::GetCacheLineSize( );
  EXPECT_GT( cacheLineSize, 0u );
  EXPECT_TRUE( Blue::IsPowerOfTwo( cacheLineSize ) );

  const Blue::ProcessorInfo info = Blue::QueryProcessorInfo( );

  EXPECT_EQ( info.Architecture, architecture );
  EXPECT_EQ( info.LogicalProcessorCount, logicalProcessorCount );
  EXPECT_EQ( info.CacheLineSize, cacheLineSize );

  ASSERT_NE( info.ArchitectureName, nullptr );
  EXPECT_NE( info.ArchitectureName[ 0 ], '\0' );
}

TEST( BlueSystemProcessorTests, RecommendedWorkerThreadCountIsAlwaysUsable )
{
  const Blue::Uint32 logicalProcessorCount = Blue::GetLogicalProcessorCount( );

  const Blue::Uint32 recommendedDefault = Blue::GetRecommendedWorkerThreadCount( );
  const Blue::Uint32 recommendedNoneReserved = Blue::GetRecommendedWorkerThreadCount( 0 );
  const Blue::Uint32 recommendedTooManyReserved = Blue::GetRecommendedWorkerThreadCount( logicalProcessorCount + 32u );

  EXPECT_GT( recommendedDefault, 0u );
  EXPECT_GT( recommendedNoneReserved, 0u );
  EXPECT_EQ( recommendedTooManyReserved, 1u );

  EXPECT_LE( recommendedDefault, logicalProcessorCount );
  EXPECT_LE( recommendedNoneReserved, logicalProcessorCount );
}

TEST( BlueSystemProcessorTests, ProcessorWaitPrimitivesAreCallable )
{
  Blue::ProcessorPause( );
  Blue::YieldThread( );
  Blue::SleepCurrentThread( 1 );

  SUCCEED( );
}

#include <Blue/System/Alignment.h>
#include <Blue/System/NonCopyable.h>
#include <Blue/System/Result.h>
#include <Blue/System/SourceLocation.h>
#include <Blue/System/Types.h>

#include <type_traits>

#include <gtest/gtest.h>

namespace
{
class NonCopyableProbe final : public Blue::NonCopyable
{};

class NonMovableProbe final : public Blue::NonMovable
{};
} // namespace

static_assert( sizeof( Blue::Bool ) == 1 );
static_assert( sizeof( Blue::Uint8 ) == 1 );
static_assert( sizeof( Blue::Uint16 ) == 2 );
static_assert( sizeof( Blue::Uint32 ) == 4 );
static_assert( sizeof( Blue::Uint64 ) == 8 );

static_assert( !std::is_copy_constructible_v< NonCopyableProbe > );
static_assert( !std::is_copy_assignable_v< NonCopyableProbe > );
static_assert( std::is_move_constructible_v< NonCopyableProbe > );
static_assert( std::is_move_assignable_v< NonCopyableProbe > );

static_assert( !std::is_copy_constructible_v< NonMovableProbe > );
static_assert( !std::is_copy_assignable_v< NonMovableProbe > );
static_assert( !std::is_move_constructible_v< NonMovableProbe > );
static_assert( !std::is_move_assignable_v< NonMovableProbe > );

TEST( BlueSystemBaseContractTests, ResultSuccessStateIsStable )
{
  const Blue::Result result = Blue::Success( );

  EXPECT_EQ( result.Code, Blue::ResultCode::Success );
  EXPECT_TRUE( result.Succeeded( ) );
  EXPECT_FALSE( result.Failed( ) );
}

TEST( BlueSystemBaseContractTests, ResultFailureStateIsStable )
{
  const Blue::Result result = Blue::Failure( Blue::ResultCode::InvalidArgument );

  EXPECT_EQ( result.Code, Blue::ResultCode::InvalidArgument );
  EXPECT_FALSE( result.Succeeded( ) );
  EXPECT_TRUE( result.Failed( ) );
}

TEST( BlueSystemBaseContractTests, PowerOfTwoDetectionIsStable )
{
  EXPECT_FALSE( Blue::IsPowerOfTwo( 0u ) );
  EXPECT_TRUE( Blue::IsPowerOfTwo( 1u ) );
  EXPECT_TRUE( Blue::IsPowerOfTwo( 2u ) );
  EXPECT_TRUE( Blue::IsPowerOfTwo( 8u ) );
  EXPECT_TRUE( Blue::IsPowerOfTwo( 64u ) );

  EXPECT_FALSE( Blue::IsPowerOfTwo( 3u ) );
  EXPECT_FALSE( Blue::IsPowerOfTwo( 6u ) );
  EXPECT_FALSE( Blue::IsPowerOfTwo( 63u ) );
}

TEST( BlueSystemBaseContractTests, AlignUpRoundsToNextAlignedValue )
{
  EXPECT_EQ( Blue::AlignUp( 0u, 8u ), 0u );
  EXPECT_EQ( Blue::AlignUp( 1u, 8u ), 8u );
  EXPECT_EQ( Blue::AlignUp( 7u, 8u ), 8u );
  EXPECT_EQ( Blue::AlignUp( 8u, 8u ), 8u );
  EXPECT_EQ( Blue::AlignUp( 9u, 8u ), 16u );
}

TEST( BlueSystemBaseContractTests, AlignDownRoundsToPreviousAlignedValue )
{
  EXPECT_EQ( Blue::AlignDown( 0u, 8u ), 0u );
  EXPECT_EQ( Blue::AlignDown( 1u, 8u ), 0u );
  EXPECT_EQ( Blue::AlignDown( 7u, 8u ), 0u );
  EXPECT_EQ( Blue::AlignDown( 8u, 8u ), 8u );
  EXPECT_EQ( Blue::AlignDown( 15u, 8u ), 8u );
}

TEST( BlueSystemBaseContractTests, InvalidAlignmentLeavesValueUnchanged )
{
  EXPECT_EQ( Blue::AlignUp( 17u, 0u ), 17u );
  EXPECT_EQ( Blue::AlignUp( 17u, 3u ), 17u );

  EXPECT_EQ( Blue::AlignDown( 17u, 0u ), 17u );
  EXPECT_EQ( Blue::AlignDown( 17u, 3u ), 17u );
}

TEST( BlueSystemBaseContractTests, AlignmentPaddingIsStable )
{
  EXPECT_EQ( Blue::GetAlignmentPadding( 0u, 8u ), 0u );
  EXPECT_EQ( Blue::GetAlignmentPadding( 1u, 8u ), 7u );
  EXPECT_EQ( Blue::GetAlignmentPadding( 7u, 8u ), 1u );
  EXPECT_EQ( Blue::GetAlignmentPadding( 8u, 8u ), 0u );
  EXPECT_EQ( Blue::GetAlignmentPadding( 9u, 8u ), 7u );
}

TEST( BlueSystemBaseContractTests, SourceLocationMacroCapturesCallSite )
{
  const Blue::SourceLocation location = BLUE_SOURCE_LOCATION( );

  ASSERT_NE( location.File, nullptr );
  ASSERT_NE( location.Function, nullptr );

  EXPECT_GT( location.Line, 0u );
  EXPECT_NE( location.File[ 0 ], '\0' );
  EXPECT_NE( location.Function[ 0 ], '\0' );
}

// Copyright (c) Khaled Shawki. All rights reserved.

#include <Blue/Memory/Backend/MemoryBackend.h>
#include <Blue/System/Types.h>

#include <gtest/gtest.h>

namespace
{
constexpr Blue::Size NormalAlignment = alignof( Blue::Uint64 );
constexpr Blue::Size OverAlignment = 64;

bool IsAligned( const void* pointer, Blue::Size alignment ) noexcept
{
  return ( reinterpret_cast< Blue::NativeUInt >( pointer ) % alignment ) == 0;
}

void FillPattern( void* pointer, Blue::Size size ) noexcept
{
  auto* bytes = static_cast< Blue::Byte* >( pointer );

  for ( Blue::Size index = 0; index < size; ++index )
  {
    bytes[ index ] = static_cast< Blue::Byte >( index & 0xFF );
  }
}

void ExpectPatternPreserved( const void* pointer, Blue::Size size )
{
  const auto* bytes = static_cast< const Blue::Byte* >( pointer );

  for ( Blue::Size index = 0; index < size; ++index )
  {
    EXPECT_EQ( bytes[ index ], static_cast< Blue::Byte >( index & 0xFF ) );
  }
}
} // namespace

TEST( BlueMemoryBackendContract, ReportsSelectedBackend )
{
#if BLUE_MEMORY_USE_MIMALLOC
  EXPECT_EQ( Blue::MemoryBackend::GetKind( ), Blue::MemoryBackendKind::Mimalloc );
  EXPECT_STREQ( Blue::MemoryBackend::GetName( ), "mimalloc" );
#else
  EXPECT_EQ( Blue::MemoryBackend::GetKind( ), Blue::MemoryBackendKind::System );
  EXPECT_STREQ( Blue::MemoryBackend::GetName( ), "system" );
#endif
}

TEST( BlueMemoryBackendContract, ReturnsNullForZeroSizeAllocation )
{
  void* pointer = Blue::MemoryBackend::Allocate( 0, NormalAlignment );

  EXPECT_EQ( pointer, nullptr );
}

TEST( BlueMemoryBackendContract, FreeNullPointerIsSafe )
{
  Blue::MemoryBackend::Free( nullptr, 0, NormalAlignment );
}

TEST( BlueMemoryBackendContract, AllocatesNormalAlignedMemory )
{
  constexpr Blue::Size Size = 128;

  void* pointer = Blue::MemoryBackend::Allocate( Size, NormalAlignment );

  ASSERT_NE( pointer, nullptr );
  EXPECT_TRUE( IsAligned( pointer, NormalAlignment ) );

  FillPattern( pointer, Size );
  ExpectPatternPreserved( pointer, Size );

  Blue::MemoryBackend::Free( pointer, Size, NormalAlignment );
}

TEST( BlueMemoryBackendContract, AllocatesOverAlignedMemory )
{
  constexpr Blue::Size Size = 256;

  void* pointer = Blue::MemoryBackend::Allocate( Size, OverAlignment );

  ASSERT_NE( pointer, nullptr );
  EXPECT_TRUE( IsAligned( pointer, OverAlignment ) );

  FillPattern( pointer, Size );
  ExpectPatternPreserved( pointer, Size );

  Blue::MemoryBackend::Free( pointer, Size, OverAlignment );
}

TEST( BlueMemoryBackendContract, ReallocateNullPointerAllocatesMemory )
{
  constexpr Blue::Size Size = 128;

  void* pointer = Blue::MemoryBackend::Reallocate( nullptr, 0, Size, OverAlignment );

  ASSERT_NE( pointer, nullptr );
  EXPECT_TRUE( IsAligned( pointer, OverAlignment ) );

  FillPattern( pointer, Size );
  ExpectPatternPreserved( pointer, Size );

  Blue::MemoryBackend::Free( pointer, Size, OverAlignment );
}

TEST( BlueMemoryBackendContract, ReallocateGrowPreservesExistingBytes )
{
  constexpr Blue::Size OldSize = 64;
  constexpr Blue::Size NewSize = 256;

  void* pointer = Blue::MemoryBackend::Allocate( OldSize, OverAlignment );

  ASSERT_NE( pointer, nullptr );
  ASSERT_TRUE( IsAligned( pointer, OverAlignment ) );

  FillPattern( pointer, OldSize );

  void* reallocated = Blue::MemoryBackend::Reallocate( pointer, OldSize, NewSize, OverAlignment );

  if ( reallocated == nullptr )
  {
    Blue::MemoryBackend::Free( pointer, OldSize, OverAlignment );
    FAIL( ) << "MemoryBackend::Reallocate failed while growing allocation.";
    return;
  }

  EXPECT_TRUE( IsAligned( reallocated, OverAlignment ) );
  ExpectPatternPreserved( reallocated, OldSize );

  Blue::MemoryBackend::Free( reallocated, NewSize, OverAlignment );
}

TEST( BlueMemoryBackendContract, ReallocateShrinkPreservesPrefixBytes )
{
  constexpr Blue::Size OldSize = 256;
  constexpr Blue::Size NewSize = 64;

  void* pointer = Blue::MemoryBackend::Allocate( OldSize, OverAlignment );

  ASSERT_NE( pointer, nullptr );
  ASSERT_TRUE( IsAligned( pointer, OverAlignment ) );

  FillPattern( pointer, OldSize );

  void* reallocated = Blue::MemoryBackend::Reallocate( pointer, OldSize, NewSize, OverAlignment );

  if ( reallocated == nullptr )
  {
    Blue::MemoryBackend::Free( pointer, OldSize, OverAlignment );
    FAIL( ) << "MemoryBackend::Reallocate failed while shrinking allocation.";
    return;
  }

  EXPECT_TRUE( IsAligned( reallocated, OverAlignment ) );
  ExpectPatternPreserved( reallocated, NewSize );

  Blue::MemoryBackend::Free( reallocated, NewSize, OverAlignment );
}

TEST( BlueMemoryBackendContract, ReallocateToZeroFreesMemory )
{
  constexpr Blue::Size Size = 128;

  void* pointer = Blue::MemoryBackend::Allocate( Size, OverAlignment );

  ASSERT_NE( pointer, nullptr );

  FillPattern( pointer, Size );

  void* reallocated = Blue::MemoryBackend::Reallocate( pointer, Size, 0, OverAlignment );

  EXPECT_EQ( reallocated, nullptr );
}

#pragma once

#include <Blue/System/Compiler.h>
#include <Blue/System/Types.h>

#include <type_traits>

namespace Blue
{
enum class MemoryOrder : Uint8
{
  Relaxed,
  Acquire,
  Release,
  AcquireRelease,
  SequentiallyConsistent,
};

enum class AtomicKind : Uint8
{
  Unsupported,
  Boolean,
  SignedInteger,
  UnsignedInteger,
  Enum,
  Pointer,
  Value128,
};

struct alignas( 16 ) AtomicValue128
{
  Uint64 Low;
  Uint64 High;

  constexpr AtomicValue128( ) noexcept
      : Low( 0 )
      , High( 0 )
  {}

  constexpr AtomicValue128( Uint64 low, Uint64 high ) noexcept
      : Low( low )
      , High( high )
  {}

  friend constexpr Bool operator==( AtomicValue128 left, AtomicValue128 right ) noexcept
  {
    return left.Low == right.Low && left.High == right.High;
  }

  friend constexpr Bool operator!=( AtomicValue128 left, AtomicValue128 right ) noexcept { return !( left == right ); }
};

static_assert( sizeof( AtomicValue128 ) == 16, "Blue::AtomicValue128 must be exactly 16 bytes." );
static_assert( alignof( AtomicValue128 ) == 16, "Blue::AtomicValue128 must be 16-byte aligned." );

template< typename T, Bool IsEnum >
struct AtomicStorageSelector
{
  using Type = T;
};

template< typename T >
struct AtomicStorageSelector< T, true >
{
  using Type = std::underlying_type_t< T >;
};

template< typename T >
struct AtomicTypePolicy
{
  static constexpr Bool IsPointer = std::is_pointer_v< T >;
  static constexpr Bool IsBool = std::is_same_v< T, Bool >;
  static constexpr Bool IsEnum = std::is_enum_v< T >;
  static constexpr Bool IsValue128 = std::is_same_v< T, AtomicValue128 >;
  static constexpr Bool IsIntegral = std::is_integral_v< T > && !IsBool;
  static constexpr Bool IsSignedInteger = IsIntegral && std::is_signed_v< T >;
  static constexpr Bool IsUnsignedInteger = IsIntegral && std::is_unsigned_v< T >;

  static constexpr AtomicKind Kind = IsPointer         ? AtomicKind::Pointer
                                   : IsBool            ? AtomicKind::Boolean
                                   : IsEnum            ? AtomicKind::Enum
                                   : IsValue128        ? AtomicKind::Value128
                                   : IsSignedInteger   ? AtomicKind::SignedInteger
                                   : IsUnsignedInteger ? AtomicKind::UnsignedInteger
                                                       : AtomicKind::Unsupported;

  using StorageType = typename AtomicStorageSelector< T, IsEnum >::Type;

  static constexpr Bool IsNativeAtomicWidth = sizeof( StorageType ) == 1 || sizeof( StorageType ) == 2 ||
                                              sizeof( StorageType ) == 4 || sizeof( StorageType ) == 8;
  static constexpr Bool IsWideAtomicWidth = sizeof( StorageType ) == 16;
  static constexpr Bool IsSupportedWidth = IsNativeAtomicWidth || IsWideAtomicWidth;

  static constexpr Bool IsSupported =
    Kind != AtomicKind::Unsupported && IsSupportedWidth && std::is_trivially_copyable_v< T >;

  static constexpr Bool SupportsFetchArithmetic =
    IsIntegral && !IsBool && ( sizeof( StorageType ) == 4 || sizeof( StorageType ) == 8 );

  static constexpr Size RequiredAlignment = sizeof( StorageType ) < 16 ? sizeof( StorageType ) : 16;

  static constexpr StorageType ToStorage( T value ) noexcept
  {
    if constexpr ( IsEnum )
    {
      return static_cast< StorageType >( value );
    }
    else
    {
      return value;
    }
  }

  static constexpr T FromStorage( StorageType value ) noexcept
  {
    if constexpr ( IsEnum )
    {
      return static_cast< T >( value );
    }
    else
    {
      return value;
    }
  }
};

BLUE_FORCE_INLINE constexpr Bool IsValidLoadOrder( MemoryOrder order ) noexcept
{
  return order == MemoryOrder::Relaxed || order == MemoryOrder::Acquire || order == MemoryOrder::SequentiallyConsistent;
}

BLUE_FORCE_INLINE constexpr Bool IsValidStoreOrder( MemoryOrder order ) noexcept
{
  return order == MemoryOrder::Relaxed || order == MemoryOrder::Release || order == MemoryOrder::SequentiallyConsistent;
}

BLUE_FORCE_INLINE constexpr MemoryOrder NormalizeLoadOrder( MemoryOrder order ) noexcept
{
  return IsValidLoadOrder( order ) ? order : MemoryOrder::SequentiallyConsistent;
}

BLUE_FORCE_INLINE constexpr MemoryOrder NormalizeStoreOrder( MemoryOrder order ) noexcept
{
  return IsValidStoreOrder( order ) ? order : MemoryOrder::SequentiallyConsistent;
}

BLUE_FORCE_INLINE constexpr Bool IsValidCompareExchangeFailureOrder( MemoryOrder order ) noexcept
{
  return order == MemoryOrder::Relaxed || order == MemoryOrder::Acquire || order == MemoryOrder::SequentiallyConsistent;
}

BLUE_FORCE_INLINE constexpr MemoryOrder GetCompareExchangeFailureOrder( MemoryOrder successOrder ) noexcept
{
  switch ( successOrder )
  {
    case MemoryOrder::Relaxed :
    case MemoryOrder::Release : return MemoryOrder::Relaxed;

    case MemoryOrder::Acquire :
    case MemoryOrder::AcquireRelease : return MemoryOrder::Acquire;

    case MemoryOrder::SequentiallyConsistent :
    default :                                  return MemoryOrder::SequentiallyConsistent;
  }
}

BLUE_FORCE_INLINE constexpr Bool IsCompareExchangeFailureOrderCompatible( MemoryOrder successOrder,
                                                                          MemoryOrder failureOrder ) noexcept
{
  switch ( failureOrder )
  {
    case MemoryOrder::Relaxed : return true;

    case MemoryOrder::Acquire :
      return successOrder == MemoryOrder::Acquire || successOrder == MemoryOrder::AcquireRelease ||
             successOrder == MemoryOrder::SequentiallyConsistent;

    case MemoryOrder::SequentiallyConsistent : return successOrder == MemoryOrder::SequentiallyConsistent;

    case MemoryOrder::Release :
    case MemoryOrder::AcquireRelease :
    default :                          return false;
  }
}

BLUE_FORCE_INLINE constexpr MemoryOrder NormalizeCompareExchangeFailureOrder( MemoryOrder successOrder,
                                                                              MemoryOrder failureOrder ) noexcept
{
  return IsValidCompareExchangeFailureOrder( failureOrder ) &&
             IsCompareExchangeFailureOrderCompatible( successOrder, failureOrder )
         ? failureOrder
         : GetCompareExchangeFailureOrder( successOrder );
}

template< typename TStorage, Size SizeOfStorage, AtomicKind Kind >
struct AtomicPrimitive;

template< typename TStorage, AtomicKind Kind >
inline constexpr Bool AlwaysFalseAtomicPrimitive = false;
} // namespace Blue

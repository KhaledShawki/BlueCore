#pragma once

#include <Blue/System/Threading/Atomic_Primitives.h>

namespace Blue
{
template< typename T >
class Atomic final
{
private:
  using Policy = AtomicTypePolicy< T >;
  using StorageType = typename Policy::StorageType;
  using Primitive = AtomicPrimitive< StorageType, sizeof( StorageType ), Policy::Kind >;

  static_assert( Policy::IsSupported,
                 "Blue::Atomic<T> supports only Bool, 1/2/4/8-byte integral types, 1/2/4/8-byte enum types, "
                 "pointer types, and Blue::AtomicValue128." );
  static_assert( !Policy::IsWideAtomicWidth || Policy::Kind == AtomicKind::Value128,
                 "16-byte Blue::Atomic<T> is supported only through Blue::AtomicValue128." );

public:
  using ValueType = T;

  constexpr Atomic( ) noexcept
      : m_Value( Policy::ToStorage( T{ } ) )
  {}

  constexpr explicit Atomic( T value ) noexcept
      : m_Value( Policy::ToStorage( value ) )
  {}

  Atomic( const Atomic& ) = delete;
  Atomic& operator=( const Atomic& ) = delete;

  T Load( MemoryOrder order = MemoryOrder::SequentiallyConsistent ) const noexcept
  {
    return Policy::FromStorage( Primitive::Load( &m_Value, NormalizeLoadOrder( order ) ) );
  }

  void Store( T value, MemoryOrder order = MemoryOrder::SequentiallyConsistent ) noexcept
  {
    Primitive::Store( &m_Value, Policy::ToStorage( value ), NormalizeStoreOrder( order ) );
  }

  T Exchange( T value, MemoryOrder order = MemoryOrder::SequentiallyConsistent ) noexcept
  {
    return Policy::FromStorage( Primitive::Exchange( &m_Value, Policy::ToStorage( value ), order ) );
  }

  Bool CompareExchange( T& expected, T desired, MemoryOrder order = MemoryOrder::SequentiallyConsistent ) noexcept
  {
    return CompareExchange( expected, desired, order, GetCompareExchangeFailureOrder( order ) );
  }

  Bool CompareExchange( T& expected, T desired, MemoryOrder successOrder, MemoryOrder failureOrder ) noexcept
  {
    StorageType expectedStorage = Policy::ToStorage( expected );
    const MemoryOrder normalizedFailureOrder = NormalizeCompareExchangeFailureOrder( successOrder, failureOrder );
    const Bool exchanged = Primitive::CompareExchange( &m_Value,
                                                       &expectedStorage,
                                                       Policy::ToStorage( desired ),
                                                       successOrder,
                                                       normalizedFailureOrder );

    expected = Policy::FromStorage( expectedStorage );
    return exchanged;
  }

  template< typename U = T >
  T FetchAdd( U value, MemoryOrder order = MemoryOrder::SequentiallyConsistent ) noexcept
    requires( Policy::SupportsFetchArithmetic && std::is_convertible_v< U, T > )
  {
    return Policy::FromStorage(
      Primitive::FetchAdd( &m_Value, Policy::ToStorage( static_cast< T >( value ) ), order ) );
  }

  template< typename U = T >
  T FetchSub( U value, MemoryOrder order = MemoryOrder::SequentiallyConsistent ) noexcept
    requires( Policy::SupportsFetchArithmetic && std::is_convertible_v< U, T > )
  {
    return Policy::FromStorage(
      Primitive::FetchSub( &m_Value, Policy::ToStorage( static_cast< T >( value ) ), order ) );
  }

private:
  alignas( Policy::RequiredAlignment ) mutable StorageType m_Value;
};

using AtomicBool = Atomic< Bool >;
using AtomicInt32 = Atomic< Int32 >;
using AtomicUint32 = Atomic< Uint32 >;
using AtomicInt64 = Atomic< Int64 >;
using AtomicUint64 = Atomic< Uint64 >;
using AtomicPointer = Atomic< void* >;
using Atomic128 = Atomic< AtomicValue128 >;

static_assert( alignof( AtomicBool ) >= alignof( Bool ) );
static_assert( alignof( AtomicInt32 ) >= 4 );
static_assert( alignof( AtomicUint32 ) >= 4 );
static_assert( alignof( AtomicInt64 ) >= 8 );
static_assert( alignof( AtomicUint64 ) >= 8 );
static_assert( alignof( AtomicPointer ) >= alignof( void* ) );
static_assert( alignof( Atomic128 ) >= 16 );
static_assert( sizeof( Atomic128 ) == 16 );
} // namespace Blue

#include <Blue/System/Threading/Atomic.inl>

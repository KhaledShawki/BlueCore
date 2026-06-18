#pragma once

namespace Blue
{
BLUE_FORCE_INLINE constexpr int ToGccClangMemoryOrder( MemoryOrder order ) noexcept
{
  switch ( order )
  {
    case MemoryOrder::Relaxed : return __ATOMIC_RELAXED;

    case MemoryOrder::Acquire : return __ATOMIC_ACQUIRE;

    case MemoryOrder::Release : return __ATOMIC_RELEASE;

    case MemoryOrder::AcquireRelease : return __ATOMIC_ACQ_REL;

    case MemoryOrder::SequentiallyConsistent :
    default :                                  return __ATOMIC_SEQ_CST;
  }
}

template< typename TStorage, Size SizeOfStorage, AtomicKind Kind >
struct AtomicPrimitive
{
  static_assert( SizeOfStorage == 1 || SizeOfStorage == 2 || SizeOfStorage == 4 || SizeOfStorage == 8 );
  static_assert( Kind != AtomicKind::Unsupported );

  static BLUE_FORCE_INLINE TStorage Load( const TStorage* value, MemoryOrder order ) noexcept
  {
    return __atomic_load_n( value, ToGccClangMemoryOrder( NormalizeLoadOrder( order ) ) );
  }

  static BLUE_FORCE_INLINE void Store( TStorage* value, TStorage desired, MemoryOrder order ) noexcept
  {
    __atomic_store_n( value, desired, ToGccClangMemoryOrder( NormalizeStoreOrder( order ) ) );
  }

  static BLUE_FORCE_INLINE TStorage Exchange( TStorage* value, TStorage desired, MemoryOrder order ) noexcept
  {
    return __atomic_exchange_n( value, desired, ToGccClangMemoryOrder( order ) );
  }

  static BLUE_FORCE_INLINE Bool CompareExchange( TStorage* value,
                                                 TStorage* expected,
                                                 TStorage desired,
                                                 MemoryOrder successOrder,
                                                 MemoryOrder failureOrder ) noexcept
  {
    return __atomic_compare_exchange_n( value,
                                        expected,
                                        desired,
                                        false,
                                        ToGccClangMemoryOrder( successOrder ),
                                        ToGccClangMemoryOrder( failureOrder ) );
  }

  static BLUE_FORCE_INLINE TStorage FetchAdd( TStorage* value, TStorage amount, MemoryOrder order ) noexcept
  {
    static_assert( Kind == AtomicKind::SignedInteger || Kind == AtomicKind::UnsignedInteger );
    return __atomic_fetch_add( value, amount, ToGccClangMemoryOrder( order ) );
  }

  static BLUE_FORCE_INLINE TStorage FetchSub( TStorage* value, TStorage amount, MemoryOrder order ) noexcept
  {
    static_assert( Kind == AtomicKind::SignedInteger || Kind == AtomicKind::UnsignedInteger );
    return __atomic_fetch_sub( value, amount, ToGccClangMemoryOrder( order ) );
  }
};

#if BLUE_COMPILER_IS_CLANG
#  pragma clang diagnostic push
#  pragma clang diagnostic ignored "-Watomic-alignment"
#endif

template< typename TStorage, AtomicKind Kind >
struct AtomicPrimitive< TStorage, 16, Kind >
{
  static_assert( Kind == AtomicKind::Value128, "16-byte Blue atomics require Blue::AtomicValue128." );
  static_assert( sizeof( TStorage ) == 16 );
  static_assert( alignof( TStorage ) >= 16 );

  static BLUE_FORCE_INLINE TStorage Load( const TStorage* value, MemoryOrder order ) noexcept
  {
    TStorage result{ };
    __atomic_load( value, &result, ToGccClangMemoryOrder( NormalizeLoadOrder( order ) ) );
    return result;
  }

  static BLUE_FORCE_INLINE void Store( TStorage* value, TStorage desired, MemoryOrder order ) noexcept
  {
    __atomic_store( value, &desired, ToGccClangMemoryOrder( NormalizeStoreOrder( order ) ) );
  }

  static BLUE_FORCE_INLINE TStorage Exchange( TStorage* value, TStorage desired, MemoryOrder order ) noexcept
  {
    TStorage result{ };
    __atomic_exchange( value, &desired, &result, ToGccClangMemoryOrder( order ) );
    return result;
  }

  static BLUE_FORCE_INLINE Bool CompareExchange( TStorage* value,
                                                 TStorage* expected,
                                                 TStorage desired,
                                                 MemoryOrder successOrder,
                                                 MemoryOrder failureOrder ) noexcept
  {
    return __atomic_compare_exchange( value,
                                      expected,
                                      &desired,
                                      false,
                                      ToGccClangMemoryOrder( successOrder ),
                                      ToGccClangMemoryOrder( failureOrder ) );
  }
};

#if BLUE_COMPILER_IS_CLANG
#  pragma clang diagnostic pop
#endif

} // namespace Blue

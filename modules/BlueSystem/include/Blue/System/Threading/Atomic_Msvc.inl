#pragma once

#include <Blue/System/Architecture.h>

#include <intrin.h>

#if BLUE_ARCH == BLUE_ARCH_X64
#	define BLUE_MSVC_SUPPORTS_INTERLOCKED_COMPARE_EXCHANGE_128 1
#else
#	define BLUE_MSVC_SUPPORTS_INTERLOCKED_COMPARE_EXCHANGE_128 0
#endif

#pragma intrinsic( _InterlockedCompareExchange8 )
#pragma intrinsic( _InterlockedExchange8 )
#pragma intrinsic( _InterlockedCompareExchange16 )
#pragma intrinsic( _InterlockedExchange16 )
#pragma intrinsic( _InterlockedCompareExchange )
#pragma intrinsic( _InterlockedExchange )
#pragma intrinsic( _InterlockedExchangeAdd )
#pragma intrinsic( _InterlockedCompareExchange64 )
#pragma intrinsic( _InterlockedExchange64 )
#pragma intrinsic( _InterlockedExchangeAdd64 )
#pragma intrinsic( _InterlockedCompareExchangePointer )
#pragma intrinsic( _InterlockedExchangePointer )

#if BLUE_MSVC_SUPPORTS_INTERLOCKED_COMPARE_EXCHANGE_128
#	pragma intrinsic( _InterlockedCompareExchange128 )
#endif

namespace Blue
{
template< typename TStorage, AtomicKind Kind >
struct AtomicPrimitive< TStorage, 1, Kind >
{
	static BLUE_FORCE_INLINE TStorage Load( const TStorage* value, MemoryOrder ) noexcept
	{
		const char result =
		    _InterlockedCompareExchange8( reinterpret_cast< volatile char* >( const_cast< TStorage* >( value ) ),
			                              0,
			                              0 );
		return static_cast< TStorage >( result );
	}

	static BLUE_FORCE_INLINE void Store( TStorage* value, TStorage desired, MemoryOrder ) noexcept
	{
		_InterlockedExchange8( reinterpret_cast< volatile char* >( value ), static_cast< char >( desired ) );
	}

	static BLUE_FORCE_INLINE TStorage Exchange( TStorage* value, TStorage desired, MemoryOrder ) noexcept
	{
		const char result =
		    _InterlockedExchange8( reinterpret_cast< volatile char* >( value ), static_cast< char >( desired ) );
		return static_cast< TStorage >( result );
	}

	static BLUE_FORCE_INLINE Bool
	CompareExchange( TStorage* value, TStorage* expected, TStorage desired, MemoryOrder, MemoryOrder ) noexcept
	{
		const char original = _InterlockedCompareExchange8( reinterpret_cast< volatile char* >( value ),
		                                                    static_cast< char >( desired ),
		                                                    static_cast< char >( *expected ) );
		const Bool exchanged = original == static_cast< char >( *expected );
		*expected = static_cast< TStorage >( original );
		return exchanged;
	}
};

template< typename TStorage, AtomicKind Kind >
struct AtomicPrimitive< TStorage, 2, Kind >
{
	static BLUE_FORCE_INLINE TStorage Load( const TStorage* value, MemoryOrder ) noexcept
	{
		const short result =
		    _InterlockedCompareExchange16( reinterpret_cast< volatile short* >( const_cast< TStorage* >( value ) ),
			                               0,
			                               0 );
		return static_cast< TStorage >( result );
	}

	static BLUE_FORCE_INLINE void Store( TStorage* value, TStorage desired, MemoryOrder ) noexcept
	{
		_InterlockedExchange16( reinterpret_cast< volatile short* >( value ), static_cast< short >( desired ) );
	}

	static BLUE_FORCE_INLINE TStorage Exchange( TStorage* value, TStorage desired, MemoryOrder ) noexcept
	{
		const short result =
		    _InterlockedExchange16( reinterpret_cast< volatile short* >( value ), static_cast< short >( desired ) );
		return static_cast< TStorage >( result );
	}

	static BLUE_FORCE_INLINE Bool
	CompareExchange( TStorage* value, TStorage* expected, TStorage desired, MemoryOrder, MemoryOrder ) noexcept
	{
		const short original = _InterlockedCompareExchange16( reinterpret_cast< volatile short* >( value ),
		                                                      static_cast< short >( desired ),
		                                                      static_cast< short >( *expected ) );
		const Bool exchanged = original == static_cast< short >( *expected );
		*expected = static_cast< TStorage >( original );
		return exchanged;
	}
};

template< typename TStorage, AtomicKind Kind >
struct AtomicPrimitive< TStorage, 4, Kind >
{
	static BLUE_FORCE_INLINE TStorage Load( const TStorage* value, MemoryOrder ) noexcept
	{
		if constexpr ( Kind == AtomicKind::Pointer )
		{
			void* result = _InterlockedCompareExchangePointer(
			    reinterpret_cast< void* volatile* >( const_cast< TStorage* >( value ) ),
			    nullptr,
			    nullptr );
			return reinterpret_cast< TStorage >( result );
		}
		else
		{
			const long result =
			    _InterlockedCompareExchange( reinterpret_cast< volatile long* >( const_cast< TStorage* >( value ) ),
				                             0,
				                             0 );
			return static_cast< TStorage >( result );
		}
	}

	static BLUE_FORCE_INLINE void Store( TStorage* value, TStorage desired, MemoryOrder ) noexcept
	{
		if constexpr ( Kind == AtomicKind::Pointer )
		{
			_InterlockedExchangePointer( reinterpret_cast< void* volatile* >( value ),
			                             reinterpret_cast< void* >( desired ) );
		}
		else
		{
			_InterlockedExchange( reinterpret_cast< volatile long* >( value ), static_cast< long >( desired ) );
		}
	}

	static BLUE_FORCE_INLINE TStorage Exchange( TStorage* value, TStorage desired, MemoryOrder ) noexcept
	{
		if constexpr ( Kind == AtomicKind::Pointer )
		{
			void* result = _InterlockedExchangePointer( reinterpret_cast< void* volatile* >( value ),
			                                            reinterpret_cast< void* >( desired ) );
			return reinterpret_cast< TStorage >( result );
		}
		else
		{
			const long result =
			    _InterlockedExchange( reinterpret_cast< volatile long* >( value ), static_cast< long >( desired ) );
			return static_cast< TStorage >( result );
		}
	}

	static BLUE_FORCE_INLINE Bool
	CompareExchange( TStorage* value, TStorage* expected, TStorage desired, MemoryOrder, MemoryOrder ) noexcept
	{
		if constexpr ( Kind == AtomicKind::Pointer )
		{
			void* original = _InterlockedCompareExchangePointer( reinterpret_cast< void* volatile* >( value ),
			                                                     reinterpret_cast< void* >( desired ),
			                                                     reinterpret_cast< void* >( *expected ) );
			const Bool exchanged = original == reinterpret_cast< void* >( *expected );
			*expected = reinterpret_cast< TStorage >( original );
			return exchanged;
		}
		else
		{
			const long original = _InterlockedCompareExchange( reinterpret_cast< volatile long* >( value ),
			                                                   static_cast< long >( desired ),
			                                                   static_cast< long >( *expected ) );
			const Bool exchanged = original == static_cast< long >( *expected );
			*expected = static_cast< TStorage >( original );
			return exchanged;
		}
	}

	static BLUE_FORCE_INLINE TStorage FetchAdd( TStorage* value, TStorage amount, MemoryOrder ) noexcept
	{
		static_assert( Kind == AtomicKind::SignedInteger || Kind == AtomicKind::UnsignedInteger );
		const long result =
		    _InterlockedExchangeAdd( reinterpret_cast< volatile long* >( value ), static_cast< long >( amount ) );
		return static_cast< TStorage >( result );
	}

	static BLUE_FORCE_INLINE TStorage FetchSub( TStorage* value, TStorage amount, MemoryOrder ) noexcept
	{
		static_assert( Kind == AtomicKind::SignedInteger || Kind == AtomicKind::UnsignedInteger );
		const long result =
		    _InterlockedExchangeAdd( reinterpret_cast< volatile long* >( value ), -static_cast< long >( amount ) );
		return static_cast< TStorage >( result );
	}
};

template< typename TStorage, AtomicKind Kind >
struct AtomicPrimitive< TStorage, 8, Kind >
{
	static BLUE_FORCE_INLINE TStorage Load( const TStorage* value, MemoryOrder ) noexcept
	{
		if constexpr ( Kind == AtomicKind::Pointer )
		{
			void* result = _InterlockedCompareExchangePointer(
			    reinterpret_cast< void* volatile* >( const_cast< TStorage* >( value ) ),
			    nullptr,
			    nullptr );
			return reinterpret_cast< TStorage >( result );
		}
		else
		{
			const __int64 result = _InterlockedCompareExchange64(
			    reinterpret_cast< volatile __int64* >( const_cast< TStorage* >( value ) ),
			    0,
			    0 );
			return static_cast< TStorage >( result );
		}
	}

	static BLUE_FORCE_INLINE void Store( TStorage* value, TStorage desired, MemoryOrder ) noexcept
	{
		if constexpr ( Kind == AtomicKind::Pointer )
		{
			_InterlockedExchangePointer( reinterpret_cast< void* volatile* >( value ),
			                             reinterpret_cast< void* >( desired ) );
		}
		else
		{
			_InterlockedExchange64( reinterpret_cast< volatile __int64* >( value ), static_cast< __int64 >( desired ) );
		}
	}

	static BLUE_FORCE_INLINE TStorage Exchange( TStorage* value, TStorage desired, MemoryOrder ) noexcept
	{
		if constexpr ( Kind == AtomicKind::Pointer )
		{
			void* result = _InterlockedExchangePointer( reinterpret_cast< void* volatile* >( value ),
			                                            reinterpret_cast< void* >( desired ) );
			return reinterpret_cast< TStorage >( result );
		}
		else
		{
			const __int64 result = _InterlockedExchange64( reinterpret_cast< volatile __int64* >( value ),
			                                               static_cast< __int64 >( desired ) );
			return static_cast< TStorage >( result );
		}
	}

	static BLUE_FORCE_INLINE Bool
	CompareExchange( TStorage* value, TStorage* expected, TStorage desired, MemoryOrder, MemoryOrder ) noexcept
	{
		if constexpr ( Kind == AtomicKind::Pointer )
		{
			void* original = _InterlockedCompareExchangePointer( reinterpret_cast< void* volatile* >( value ),
			                                                     reinterpret_cast< void* >( desired ),
			                                                     reinterpret_cast< void* >( *expected ) );
			const Bool exchanged = original == reinterpret_cast< void* >( *expected );
			*expected = reinterpret_cast< TStorage >( original );
			return exchanged;
		}
		else
		{
			const __int64 original = _InterlockedCompareExchange64( reinterpret_cast< volatile __int64* >( value ),
			                                                        static_cast< __int64 >( desired ),
			                                                        static_cast< __int64 >( *expected ) );
			const Bool exchanged = original == static_cast< __int64 >( *expected );
			*expected = static_cast< TStorage >( original );
			return exchanged;
		}
	}

	static BLUE_FORCE_INLINE TStorage FetchAdd( TStorage* value, TStorage amount, MemoryOrder ) noexcept
	{
		static_assert( Kind == AtomicKind::SignedInteger || Kind == AtomicKind::UnsignedInteger );
		const __int64 result = _InterlockedExchangeAdd64( reinterpret_cast< volatile __int64* >( value ),
		                                                  static_cast< __int64 >( amount ) );
		return static_cast< TStorage >( result );
	}

	static BLUE_FORCE_INLINE TStorage FetchSub( TStorage* value, TStorage amount, MemoryOrder ) noexcept
	{
		static_assert( Kind == AtomicKind::SignedInteger || Kind == AtomicKind::UnsignedInteger );
		const __int64 result = _InterlockedExchangeAdd64( reinterpret_cast< volatile __int64* >( value ),
		                                                  -static_cast< __int64 >( amount ) );
		return static_cast< TStorage >( result );
	}
};

#if BLUE_MSVC_SUPPORTS_INTERLOCKED_COMPARE_EXCHANGE_128
template< typename TStorage, AtomicKind Kind >
struct AtomicPrimitive< TStorage, 16, Kind >
{
	static_assert( Kind == AtomicKind::Value128, "16-byte Blue atomics require Blue::AtomicValue128." );
	static_assert( sizeof( TStorage ) == 16 );
	static_assert( alignof( TStorage ) >= 16 );

	static BLUE_FORCE_INLINE volatile __int64* ToNativePointer( TStorage* value ) noexcept
	{
		return reinterpret_cast< volatile __int64* >( value );
	}

	static BLUE_FORCE_INLINE volatile __int64* ToNativePointer( const TStorage* value ) noexcept
	{
		return reinterpret_cast< volatile __int64* >( const_cast< TStorage* >( value ) );
	}

	static BLUE_FORCE_INLINE void WriteComparand( __int64 ( &comparand )[ 2 ], const TStorage& value ) noexcept
	{
		comparand[ 0 ] = static_cast< __int64 >( value.Low );
		comparand[ 1 ] = static_cast< __int64 >( value.High );
	}

	static BLUE_FORCE_INLINE TStorage ReadComparand( const __int64 ( &comparand )[ 2 ] ) noexcept
	{
		return TStorage{ static_cast< Uint64 >( comparand[ 0 ] ), static_cast< Uint64 >( comparand[ 1 ] ) };
	}

	static BLUE_FORCE_INLINE TStorage Load( const TStorage* value, MemoryOrder ) noexcept
	{
		__int64 comparand[ 2 ] = { };
		_InterlockedCompareExchange128( ToNativePointer( value ), 0, 0, comparand );
		return ReadComparand( comparand );
	}

	static BLUE_FORCE_INLINE void Store( TStorage* value, TStorage desired, MemoryOrder order ) noexcept
	{
		TStorage expected = Load( value, order );
		while ( !CompareExchange( value, &expected, desired, order, GetCompareExchangeFailureOrder( order ) ) )
		{
		}
	}

	static BLUE_FORCE_INLINE TStorage Exchange( TStorage* value, TStorage desired, MemoryOrder order ) noexcept
	{
		TStorage expected = Load( value, order );
		while ( !CompareExchange( value, &expected, desired, order, GetCompareExchangeFailureOrder( order ) ) )
		{
		}

		return expected;
	}

	static BLUE_FORCE_INLINE Bool
	CompareExchange( TStorage* value, TStorage* expected, TStorage desired, MemoryOrder, MemoryOrder ) noexcept
	{
		__int64 comparand[ 2 ];
		WriteComparand( comparand, *expected );

		const unsigned char exchanged = _InterlockedCompareExchange128( ToNativePointer( value ),
		                                                                static_cast< __int64 >( desired.High ),
		                                                                static_cast< __int64 >( desired.Low ),
		                                                                comparand );

		*expected = ReadComparand( comparand );
		return exchanged != 0;
	}
};
#else
template< typename TStorage, AtomicKind Kind >
struct AtomicPrimitive< TStorage, 16, Kind >
{
	static_assert( AlwaysFalseAtomicPrimitive< TStorage, Kind >,
	               "16-byte Blue atomics require _InterlockedCompareExchange128 support on MSVC." );
};
#endif

} // namespace Blue

#undef BLUE_MSVC_SUPPORTS_INTERLOCKED_COMPARE_EXCHANGE_128

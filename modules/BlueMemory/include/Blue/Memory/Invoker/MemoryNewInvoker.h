#pragma once

#include <Blue/Memory/AllocationFailurePolicy.h>
#include <Blue/Memory/Pool/MemoryPoolResolver.h>
#include <Blue/Memory/Proxy/TypedAllocationProxy.h>
#include <Blue/System/Assert.h>
#include <Blue/System/SourceLocation.h>

#include <new>
#include <type_traits>
#include <utility>

namespace Blue
{
template< typename T, MemoryPoolId Pool >
struct ExplicitPoolMemoryNewInvoker
{
	template< typename... Args >
	static T* TryNew( Args&&... args ) noexcept
	{
		static_assert( !std::is_array_v< T >, "BlueTryNew does not support arrays in v1." );
		static_assert( std::is_nothrow_constructible_v< T, Args... >, "BlueTryNew requires noexcept construction." );

		void* memory = TypedAllocationProxy< T, Pool >::Allocate( AllocationTag::Object, BLUE_SOURCE_LOCATION( ) );
		if ( !memory )
		{
			return nullptr;
		}

		return ::new ( memory ) T( std::forward< Args >( args )... );
	}

	template< typename... Args >
	static T* New( Args&&... args ) noexcept
	{
		T* object = TryNew( std::forward< Args >( args )... );
		if ( !object )
		{
			HandleAllocationFailure( MakeAllocationFailureInfo( Pool,
			                                                    MemoryPoolPolicy< Pool >::Allocator,
			                                                    AllocationTag::Object,
			                                                    sizeof( T ),
			                                                    alignof( T ),
			                                                    AllocationFailureReason::OutOfMemory,
			                                                    BLUE_SOURCE_LOCATION( ) ),
			                         AllocationFailurePolicy::CallHandlerThenAbort );
		}

		return object;
	}

	static void Delete( T* object ) noexcept
	{
		static_assert( !std::is_array_v< T >, "BlueDelete does not support arrays in v1." );
		static_assert( std::is_nothrow_destructible_v< T >, "BlueDelete requires noexcept destruction." );

		if ( !object )
		{
			return;
		}

		object->~T( );
		TypedAllocationProxy< T, Pool >::Free( object );
	}
};

template< typename T >
struct MemoryNewInvoker
{
	static constexpr MemoryPoolId Pool = MemoryPoolResolver< T >::Pool;

	template< typename... Args >
	static T* TryNew( Args&&... args ) noexcept
	{
		return ExplicitPoolMemoryNewInvoker< T, Pool >::TryNew( std::forward< Args >( args )... );
	}

	template< typename... Args >
	static T* New( Args&&... args ) noexcept
	{
		return ExplicitPoolMemoryNewInvoker< T, Pool >::New( std::forward< Args >( args )... );
	}

	static void Delete( T* object ) noexcept
	{
		ExplicitPoolMemoryNewInvoker< T, Pool >::Delete( object );
	}
};
} // namespace Blue

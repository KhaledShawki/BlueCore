#pragma once

#include <Blue/Memory/Invoker/MemoryNewInvoker.h>

namespace Blue
{
template< typename T, typename... Args >
T* BlueTryNew( Args&&... args ) noexcept
{
	return MemoryNewInvoker< T >::TryNew( std::forward< Args >( args )... );
}

template< typename T, typename... Args >
T* BlueNew( Args&&... args ) noexcept
{
	return MemoryNewInvoker< T >::New( std::forward< Args >( args )... );
}

template< typename T >
void BlueDelete( T* object ) noexcept
{
	MemoryNewInvoker< T >::Delete( object );
}

template< MemoryPoolId Pool, typename T, typename... Args >
T* BlueTryNewInPool( Args&&... args ) noexcept
{
	return ExplicitPoolMemoryNewInvoker< T, Pool >::TryNew( std::forward< Args >( args )... );
}

template< MemoryPoolId Pool, typename T, typename... Args >
T* BlueNewInPool( Args&&... args ) noexcept
{
	return ExplicitPoolMemoryNewInvoker< T, Pool >::New( std::forward< Args >( args )... );
}

template< MemoryPoolId Pool, typename T >
void BlueDeleteFromPool( T* object ) noexcept
{
	ExplicitPoolMemoryNewInvoker< T, Pool >::Delete( object );
}
} // namespace Blue

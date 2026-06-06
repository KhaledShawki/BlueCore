#pragma once

#include <Blue/System/Assert.h>
#include <Blue/System/Types.h>

namespace Blue
{
class StringView
{
public:
	StringView( )
	    : m_Data( nullptr )
	    , m_Size( 0 )
	{}

	StringView( const char* data, Size size )
	    : m_Data( data )
	    , m_Size( size )
	{}

	const char* Data( ) const
	{
		return m_Data;
	}

	Size GetSize( ) const
	{
		return m_Size;
	}

	bool Empty( ) const
	{
		return m_Size == 0;
	}

	const char& operator[]( Size index ) const
	{
		BLUE_ASSERT( index < m_Size );
		return m_Data[ index ];
	}

private:
	const char* m_Data;
	Size m_Size;
};
} // namespace Blue

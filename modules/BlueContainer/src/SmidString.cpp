#include <Blue/Container/SmidString.h>

#include <string.h>

namespace Blue
{
static Size CStringLength( const char* text )
{
	return text ? static_cast< Size >( strlen( text ) ) : 0;
}

SmidString::SmidString( Allocator allocator )
    : m_Storage{ }
    , m_Size( 0 )
    , m_Capacity( InlineCapacity )
    , m_Allocator( allocator )
    , m_IsInline( true )
{
	m_Storage.Inline[ 0 ] = '\0';
}

SmidString::SmidString( const char* text, Allocator allocator )
    : SmidString( text, CStringLength( text ), allocator )
{}

SmidString::SmidString( const char* text, Size size, Allocator allocator )
    : SmidString( allocator )
{
	Append( text, size );
}

SmidString::SmidString( const SmidString& other, Allocator allocator )
    : SmidString( allocator )
{
	Append( other.Data( ), other.GetSize( ) );
}

SmidString::SmidString( SmidString&& other ) noexcept
    : m_Storage{ }
    , m_Size( other.m_Size )
    , m_Capacity( other.m_Capacity )
    , m_Allocator( other.m_Allocator )
    , m_IsInline( other.m_IsInline )
{
	if ( other.m_IsInline )
	{
		memcpy( m_Storage.Inline, other.m_Storage.Inline, InlineCapacity + 1 );
	}

	other.m_Storage.Inline[ 0 ] = '\0';
	other.m_Size = 0;
	other.m_Capacity = InlineCapacity;
	other.m_IsInline = true;
	other.m_Allocator = { };
}

SmidString::~SmidString( )
{
	if ( !m_IsInline && m_Storage.Heap )
	{
		Blue::Free( m_Allocator, m_Storage.Heap, m_Capacity + 1, alignof( char ) );
	}
}

SmidString& SmidString::operator=( SmidString&& other ) noexcept
{
	if ( this == &other )
	{
		return *this;
	}

	this->~SmidString( );
	m_Storage = other.m_Storage;
	m_Size = other.m_Size;
	m_Capacity = other.m_Capacity;
	m_Allocator = other.m_Allocator;
	m_IsInline = other.m_IsInline;

	if ( other.m_IsInline )
	{
		memcpy( m_Storage.Inline, other.m_Storage.Inline, InlineCapacity + 1 );
	}

	other.m_Storage.Inline[ 0 ] = '\0';
	other.m_Size = 0;
	other.m_Capacity = InlineCapacity;
	other.m_IsInline = true;
	other.m_Allocator = { };
	return *this;
}

const char* SmidString::Data( ) const
{
	return m_IsInline ? m_Storage.Inline : m_Storage.Heap;
}

char* SmidString::Data( )
{
	return m_IsInline ? m_Storage.Inline : m_Storage.Heap;
}

Size SmidString::GetSize( ) const
{
	return m_Size;
}

Size SmidString::GetCapacity( ) const
{
	return m_Capacity;
}

bool SmidString::Empty( ) const
{
	return m_Size == 0;
}

bool SmidString::IsInline( ) const
{
	return m_IsInline;
}

void SmidString::Clear( )
{
	m_Size = 0;
	Data( )[ 0 ] = '\0';
}

bool SmidString::Reserve( Size newCapacity )
{
	if ( newCapacity <= m_Capacity )
	{
		return true;
	}

	AllocationResult result =
	    Blue::Allocate( m_Allocator,
		                BLUE_ALLOCATION_REQUEST( newCapacity + 1, alignof( char ), AllocationTag::String ) );

	if ( !result.Pointer )
	{
		return false;
	}

	char* newData = static_cast< char* >( result.Pointer );
	memcpy( newData, Data( ), m_Size + 1 );

	if ( !m_IsInline )
	{
		Blue::Free( m_Allocator, m_Storage.Heap, m_Capacity + 1, alignof( char ) );
	}

	m_Storage.Heap = newData;
	m_Capacity = newCapacity;
	m_IsInline = false;
	return true;
}

bool SmidString::Append( const char* text, Size size )
{
	if ( !text || size == 0 )
	{
		return true;
	}

	const Size required = m_Size + size;
	if ( required > m_Capacity && !Reserve( required * 2 ) )
	{
		return false;
	}

	memcpy( Data( ) + m_Size, text, size );
	m_Size += size;
	Data( )[ m_Size ] = '\0';
	return true;
}

StringView SmidString::View( ) const
{
	return StringView( Data( ), m_Size );
}
} // namespace Blue

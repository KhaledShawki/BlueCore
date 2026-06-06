#include <Blue/Memory/BlueNew.h>
#include <Blue/Memory/Pool/MemoryPoolResolver.h>
#include <Blue/Memory/Pool/MemoryPoolTrait.h>
#include <Blue/System/Types.h>

#include <stdio.h>
#include <stdlib.h>

#define BLUE_TEST_EXPECT( expression )                                                                                 \
	do                                                                                                                 \
	{                                                                                                                  \
		if ( !( expression ) )                                                                                         \
		{                                                                                                              \
			fprintf( stderr, "Test failed: %s at %s:%d\n", #expression, __FILE__, __LINE__ );                          \
			abort( );                                                                                                  \
		}                                                                                                              \
	}                                                                                                                  \
	while ( false )

namespace
{
struct RendererObject
{
	BLUE_USE_MEMORY_POOL( Renderer )
};

struct PlainObject
{};

struct TraitObject
{};
} // namespace

namespace Blue
{
template<>
struct MemoryPoolTrait< TraitObject >
{
	static constexpr MemoryPoolId Pool = MemoryPoolId::Resources;
};
} // namespace Blue

static_assert( Blue::MemoryPoolResolver< RendererObject >::Pool == Blue::MemoryPoolId::Renderer );
static_assert( Blue::MemoryPoolResolver< PlainObject >::Pool == Blue::MemoryPoolId::System );
static_assert( Blue::MemoryPoolResolver< TraitObject >::Pool == Blue::MemoryPoolId::Resources );

int main( )
{
	BLUE_TEST_EXPECT( Blue::MemoryPoolResolver< RendererObject >::Pool == Blue::MemoryPoolId::Renderer );
	BLUE_TEST_EXPECT( Blue::MemoryPoolResolver< PlainObject >::Pool == Blue::MemoryPoolId::System );
	BLUE_TEST_EXPECT( Blue::MemoryPoolResolver< TraitObject >::Pool == Blue::MemoryPoolId::Resources );

	printf( "BlueMemory pool resolver tests passed.\n" );
	return 0;
}

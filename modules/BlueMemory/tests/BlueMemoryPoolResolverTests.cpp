#include <Blue/Memory/BlueNew.h>
#include <Blue/Memory/Pool/MemoryPoolResolver.h>
#include <Blue/Memory/Pool/MemoryPoolTrait.h>
#include <Blue/System/Types.h>

#include <gtest/gtest.h>


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

TEST( BlueMemoryPoolResolverTests, RunsSuccessfully )
{
  ASSERT_TRUE( Blue::MemoryPoolResolver< RendererObject >::Pool == Blue::MemoryPoolId::Renderer );
  ASSERT_TRUE( Blue::MemoryPoolResolver< PlainObject >::Pool == Blue::MemoryPoolId::System );
  ASSERT_TRUE( Blue::MemoryPoolResolver< TraitObject >::Pool == Blue::MemoryPoolId::Resources );
}

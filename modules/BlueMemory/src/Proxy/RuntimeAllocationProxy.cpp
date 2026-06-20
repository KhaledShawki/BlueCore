#include <Blue/Memory/Proxy/RuntimeAllocationProxy.h>

#include <string.h>

namespace Blue
{
namespace
{
void* AllocateRuntimeDefaultPool( MemoryPoolId pool, const AllocationRequest& request ) noexcept
{
  switch ( pool )
  {
#define BLUE_MEMORY_POOL( Id, NameText, BudgetValue, AllocatorToken, MetricsToken, OomValue )                          \
  case MemoryPoolId::Id :                                                                                              \
    return AllocatorProxy< MemoryPoolPolicy< MemoryPoolId::Id >::Allocator, MemoryPoolId::Id >::Allocate(              \
      request.ByteSize,                                                                                                \
      request.Alignment,                                                                                               \
      request.Tag,                                                                                                     \
      { request.File, request.Function, request.Line },                                                                \
      request.Flags );
#include <Blue/Memory/Pool/MemoryPools.def>
#undef BLUE_MEMORY_POOL
    case MemoryPoolId::Count :
    default :                  return nullptr;
  }
}

void FreeRuntimeDefaultPool( MemoryPoolId pool, const AllocationFreeRequest& request ) noexcept
{
  switch ( pool )
  {
#define BLUE_MEMORY_POOL( Id, NameText, BudgetValue, AllocatorToken, MetricsToken, OomValue )                          \
  case MemoryPoolId::Id :                                                                                              \
    AllocatorProxy< MemoryPoolPolicy< MemoryPoolId::Id >::Allocator, MemoryPoolId::Id >::Free( request.Pointer,        \
                                                                                               request.ByteSize,       \
                                                                                               request.Alignment );    \
    return;
#include <Blue/Memory/Pool/MemoryPools.def>
#undef BLUE_MEMORY_POOL
    case MemoryPoolId::Count :
    default :                  return;
  }
}
} // namespace

void* RuntimeAllocationProxy::Allocate( const AllocationRequest& request ) noexcept
{
  void* pointer = AllocateRuntimeDefaultPool( request.Pool, request );
  if ( pointer && HasAllocationFlag( request.Flags, AllocationFlag_ZeroMemory ) )
  {
    memset( pointer, 0, request.ByteSize );
  }

  return pointer;
}

void RuntimeAllocationProxy::Free( const AllocationFreeRequest& request ) noexcept
{
  FreeRuntimeDefaultPool( request.Pool, request );
}
} // namespace Blue

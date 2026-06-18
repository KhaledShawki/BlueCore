#pragma once

#include <Blue/Memory/AllocatorKind.h>
#include <Blue/Memory/MemoryMetricsMode.h>
#include <Blue/Memory/MemoryUnits.h>
#include <Blue/Memory/Pool/MemoryPoolDesc.h>

namespace Blue
{
template< MemoryPoolId Pool >
struct MemoryPoolPolicy;

#define BLUE_MEMORY_POOL( Id, NameText, BudgetValue, AllocatorToken, MetricsToken, OomValue )                          \
  template<>                                                                                                           \
  struct MemoryPoolPolicy< MemoryPoolId::Id >                                                                          \
  {                                                                                                                    \
    static constexpr MemoryPoolId IdValue = MemoryPoolId::Id;                                                          \
    static constexpr const Char* Name = NameText;                                                                      \
    static constexpr Size BudgetBytes = BudgetValue;                                                                   \
    static constexpr AllocatorKind Allocator = AllocatorKind::AllocatorToken;                                          \
    static constexpr MemoryMetricsMode Metrics = MemoryMetricsMode::MetricsToken;                                      \
    static constexpr Bool EnableOomReports = OomValue;                                                                 \
    static constexpr MemoryPoolDesc GetDesc( ) noexcept                                                                \
    {                                                                                                                  \
      return MemoryPoolDesc{ IdValue, Name, BudgetBytes, Allocator, Metrics, EnableOomReports };                       \
    }                                                                                                                  \
  };
#include <Blue/Memory/Pool/MemoryPools.def>
#undef BLUE_MEMORY_POOL
} // namespace Blue

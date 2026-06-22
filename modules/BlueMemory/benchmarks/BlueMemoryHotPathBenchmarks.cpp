// Copyright (c) Khaled Shawki. All rights reserved.

#include <benchmark/benchmark.h>


namespace
{
void BM_Placeholder( benchmark::State& state )
{
  for ( auto _ : state )
  {
    benchmark::DoNotOptimize( state.iterations( ) );
  }
}
} // namespace


BENCHMARK( BM_Placeholder );

BENCHMARK_MAIN( );

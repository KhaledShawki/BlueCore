// Copyright (c) Khaled Shawki. All rights reserved.

#include <Blue/Memory/Allocation/AllocationValidation.h>
#include <Blue/Memory/Allocator/SmallBlockAllocator.h>
#include <Blue/Memory/BlueNew.h>
#include <Blue/Memory/Invoker/RuntimeAllocationInvoker.h>
#include <Blue/Memory/MemorySystem.h>
#include <Blue/Memory/Pool/MemoryPoolTrait.h>
#include <Blue/System/Types.h>

#include <cstdint>
#include <new>

#include <benchmark/benchmark.h>


namespace
{
constexpr Blue::Size RuntimeAlignment = 16;
constexpr Blue::Size SmallBlockAlignment = 16;
constexpr Blue::Size BatchSize = 1024;

static_assert( BatchSize > 0 );
static_assert( RuntimeAlignment >= 16 );
static_assert( SmallBlockAlignment >= 16 );

struct MemorySystemScope
{
  MemorySystemScope( ) noexcept
  {
    Blue::MemorySystemDesc desc = { };
    desc.EnableMetrics = false;
    desc.EnableTracking = false;
    desc.EnableLeakDetection = false;
    desc.DefaultMetricsMode = Blue::MemoryMetricsMode::Disabled;

    Initialized = Blue::InitializeMemorySystem( desc ).Succeeded( );
  }

  ~MemorySystemScope( ) noexcept
  {
    if ( Initialized )
    {
      Blue::ShutdownMemorySystem( );
    }
  }

  MemorySystemScope( const MemorySystemScope& ) = delete;
  MemorySystemScope& operator=( const MemorySystemScope& ) = delete;
  MemorySystemScope( MemorySystemScope&& ) = delete;
  MemorySystemScope& operator=( MemorySystemScope&& ) = delete;

  bool Initialized = false;
};

struct SmallBenchmarkObject
{
  BLUE_USE_MEMORY_POOL( Test )

  explicit SmallBenchmarkObject( Blue::Uint32 value ) noexcept
      : Value( value )
  {}

  Blue::Uint32 Value = 0;
};

struct MediumBenchmarkObject
{
  BLUE_USE_MEMORY_POOL( Test )

  MediumBenchmarkObject( ) noexcept
  {
    for ( Blue::Size index = 0; index < sizeof( Data ); ++index )
    {
      Data[ index ] = static_cast< Blue::Uint8 >( index );
    }
  }

  Blue::Uint8 Data[ 64 ] = { };
};

static_assert( sizeof( SmallBenchmarkObject ) <= Blue::BlueSmallBlockMaxSize );
static_assert( sizeof( MediumBenchmarkObject ) <= Blue::BlueSmallBlockMaxSize );

Blue::AllocationRequest MakeRuntimeRequest( const Blue::Size byteSize ) noexcept
{
  return BLUE_POOL_ALLOCATION_REQUEST( byteSize,
                                       RuntimeAlignment,
                                       Blue::AllocationTag::Test,
                                       Blue::MemoryPoolId::Test );
}

void FreeRuntimeAllocation( void* pointer, const Blue::Size byteSize ) noexcept
{
  Blue::BlueFree( Blue::AllocationFreeRequest{ pointer,
                                               byteSize,
                                               RuntimeAlignment,
                                               Blue::MemoryPoolId::Test,
                                               Blue::AllocationTag::Test } );
}

void* StdAllocate( const Blue::Size byteSize )
{
  return ::operator new( byteSize, std::align_val_t{ RuntimeAlignment } );
}

void StdFree( void* pointer ) noexcept
{
  ::operator delete( pointer, std::align_val_t{ RuntimeAlignment } );
}

bool PrepareMemorySystem( benchmark::State& state, const MemorySystemScope& memorySystem )
{
  if ( !memorySystem.Initialized )
  {
    state.SkipWithError( "BlueMemory system initialization failed." );
    return false;
  }

  return true;
}

bool VerifyRuntimeAllocationPath( benchmark::State& state, const Blue::Size byteSize )
{
  Blue::AllocationRequest request = MakeRuntimeRequest( byteSize );
  void* pointer = Blue::BlueTryAllocate( request );

  if ( pointer == nullptr )
  {
    state.SkipWithError( "BlueTryAllocate returned nullptr during benchmark setup." );
    return false;
  }

  FreeRuntimeAllocation( pointer, byteSize );
  return true;
}

bool VerifyStdAllocationPath( benchmark::State& state, const Blue::Size byteSize )
{
  void* pointer = StdAllocate( byteSize );

  if ( pointer == nullptr )
  {
    state.SkipWithError( "std::operator new returned nullptr during benchmark setup." );
    return false;
  }

  StdFree( pointer );
  return true;
}

bool VerifySmallBlockPath( benchmark::State& state, const Blue::Size byteSize )
{
  if ( !Blue::IsSmallBlockAllocationSupported( byteSize, SmallBlockAlignment ) )
  {
    state.SkipWithError( "Requested size/alignment is not supported by the small-block allocator." );
    return false;
  }

  void* pointer = Blue::AllocateSmallBlock( byteSize, SmallBlockAlignment );

  if ( pointer == nullptr )
  {
    state.SkipWithError( "AllocateSmallBlock returned nullptr during benchmark setup." );
    return false;
  }

  Blue::FreeSmallBlock( pointer, byteSize, SmallBlockAlignment );
  return true;
}

bool VerifyBlueNewSmallObjectPath( benchmark::State& state )
{
  SmallBenchmarkObject* object = Blue::BlueTryNew< SmallBenchmarkObject >( 42u );

  if ( object == nullptr )
  {
    state.SkipWithError( "BlueTryNew<SmallBenchmarkObject> returned nullptr during benchmark setup." );
    return false;
  }

  Blue::BlueDelete( object );
  return true;
}

bool VerifyBlueNewMediumObjectPath( benchmark::State& state )
{
  MediumBenchmarkObject* object = Blue::BlueTryNew< MediumBenchmarkObject >( );

  if ( object == nullptr )
  {
    state.SkipWithError( "BlueTryNew<MediumBenchmarkObject> returned nullptr during benchmark setup." );
    return false;
  }

  Blue::BlueDelete( object );
  return true;
}

bool VerifyStdNewSmallObjectPath( benchmark::State& state )
{
  SmallBenchmarkObject* object = new SmallBenchmarkObject( 42u );

  if ( object == nullptr )
  {
    state.SkipWithError( "new SmallBenchmarkObject returned nullptr during benchmark setup." );
    return false;
  }

  delete object;
  return true;
}

bool VerifyStdNewMediumObjectPath( benchmark::State& state )
{
  MediumBenchmarkObject* object = new MediumBenchmarkObject( );

  if ( object == nullptr )
  {
    state.SkipWithError( "new MediumBenchmarkObject returned nullptr during benchmark setup." );
    return false;
  }

  delete object;
  return true;
}

void WarmUpRuntimeAllocator( const Blue::Size byteSize ) noexcept
{
  Blue::AllocationRequest request = MakeRuntimeRequest( byteSize );

  for ( Blue::Size index = 0; index < 4096; ++index )
  {
    void* pointer = Blue::BlueTryAllocate( request );
    benchmark::DoNotOptimize( pointer );
    FreeRuntimeAllocation( pointer, byteSize );
  }
}

void WarmUpStdAllocator( const Blue::Size byteSize )
{
  for ( Blue::Size index = 0; index < 4096; ++index )
  {
    void* pointer = StdAllocate( byteSize );
    benchmark::DoNotOptimize( pointer );
    StdFree( pointer );
  }
}

void WarmUpSmallBlockAllocator( const Blue::Size byteSize ) noexcept
{
  for ( Blue::Size index = 0; index < 4096; ++index )
  {
    void* pointer = Blue::AllocateSmallBlock( byteSize, SmallBlockAlignment );
    benchmark::DoNotOptimize( pointer );
    Blue::FreeSmallBlock( pointer, byteSize, SmallBlockAlignment );
  }
}

void BM_RuntimeAllocateFree( benchmark::State& state )
{
  const Blue::Size byteSize = static_cast< Blue::Size >( state.range( 0 ) );

  MemorySystemScope memorySystem;
  if ( !PrepareMemorySystem( state, memorySystem ) || !VerifyRuntimeAllocationPath( state, byteSize ) )
  {
    return;
  }

  WarmUpRuntimeAllocator( byteSize );

  Blue::AllocationRequest request = MakeRuntimeRequest( byteSize );

  for ( auto _ : state )
  {
    void* pointer = Blue::BlueTryAllocate( request );
    benchmark::DoNotOptimize( pointer );
    FreeRuntimeAllocation( pointer, byteSize );
  }

  state.SetItemsProcessed( state.iterations( ) );
  state.SetBytesProcessed( static_cast< std::int64_t >( state.iterations( ) ) *
                           static_cast< std::int64_t >( byteSize ) );
}

void BM_StdOperatorNewDelete( benchmark::State& state )
{
  const Blue::Size byteSize = static_cast< Blue::Size >( state.range( 0 ) );

  if ( !VerifyStdAllocationPath( state, byteSize ) )
  {
    return;
  }

  WarmUpStdAllocator( byteSize );

  for ( auto _ : state )
  {
    void* pointer = StdAllocate( byteSize );
    benchmark::DoNotOptimize( pointer );
    StdFree( pointer );
  }

  state.SetItemsProcessed( state.iterations( ) );
  state.SetBytesProcessed( static_cast< std::int64_t >( state.iterations( ) ) *
                           static_cast< std::int64_t >( byteSize ) );
}

void BM_BlueTryNewDeleteSmallObject( benchmark::State& state )
{
  MemorySystemScope memorySystem;
  if ( !PrepareMemorySystem( state, memorySystem ) || !VerifyBlueNewSmallObjectPath( state ) )
  {
    return;
  }

  for ( auto _ : state )
  {
    SmallBenchmarkObject* object = Blue::BlueTryNew< SmallBenchmarkObject >( 42u );
    benchmark::DoNotOptimize( object );
    benchmark::DoNotOptimize( object->Value );
    Blue::BlueDelete( object );
  }

  state.SetItemsProcessed( state.iterations( ) );
  state.SetBytesProcessed( static_cast< std::int64_t >( state.iterations( ) ) *
                           static_cast< std::int64_t >( sizeof( SmallBenchmarkObject ) ) );
}

void BM_StdNewDeleteSmallObject( benchmark::State& state )
{
  if ( !VerifyStdNewSmallObjectPath( state ) )
  {
    return;
  }

  for ( auto _ : state )
  {
    SmallBenchmarkObject* object = new SmallBenchmarkObject( 42u );
    benchmark::DoNotOptimize( object );
    benchmark::DoNotOptimize( object->Value );
    delete object;
  }

  state.SetItemsProcessed( state.iterations( ) );
  state.SetBytesProcessed( static_cast< std::int64_t >( state.iterations( ) ) *
                           static_cast< std::int64_t >( sizeof( SmallBenchmarkObject ) ) );
}

void BM_BlueTryNewDeleteMediumObject( benchmark::State& state )
{
  MemorySystemScope memorySystem;
  if ( !PrepareMemorySystem( state, memorySystem ) || !VerifyBlueNewMediumObjectPath( state ) )
  {
    return;
  }

  for ( auto _ : state )
  {
    MediumBenchmarkObject* object = Blue::BlueTryNew< MediumBenchmarkObject >( );
    benchmark::DoNotOptimize( object );
    benchmark::DoNotOptimize( object->Data[ 0 ] );
    Blue::BlueDelete( object );
  }

  state.SetItemsProcessed( state.iterations( ) );
  state.SetBytesProcessed( static_cast< std::int64_t >( state.iterations( ) ) *
                           static_cast< std::int64_t >( sizeof( MediumBenchmarkObject ) ) );
}

void BM_StdNewDeleteMediumObject( benchmark::State& state )
{
  if ( !VerifyStdNewMediumObjectPath( state ) )
  {
    return;
  }

  for ( auto _ : state )
  {
    MediumBenchmarkObject* object = new MediumBenchmarkObject( );
    benchmark::DoNotOptimize( object );
    benchmark::DoNotOptimize( object->Data[ 0 ] );
    delete object;
  }

  state.SetItemsProcessed( state.iterations( ) );
  state.SetBytesProcessed( static_cast< std::int64_t >( state.iterations( ) ) *
                           static_cast< std::int64_t >( sizeof( MediumBenchmarkObject ) ) );
}

void BM_SmallBlockAllocateFree( benchmark::State& state )
{
  const Blue::Size byteSize = static_cast< Blue::Size >( state.range( 0 ) );

  MemorySystemScope memorySystem;
  if ( !PrepareMemorySystem( state, memorySystem ) || !VerifySmallBlockPath( state, byteSize ) )
  {
    return;
  }

  WarmUpSmallBlockAllocator( byteSize );

  for ( auto _ : state )
  {
    void* pointer = Blue::AllocateSmallBlock( byteSize, SmallBlockAlignment );
    benchmark::DoNotOptimize( pointer );
    Blue::FreeSmallBlock( pointer, byteSize, SmallBlockAlignment );
  }

  state.SetItemsProcessed( state.iterations( ) );
  state.SetBytesProcessed( static_cast< std::int64_t >( state.iterations( ) ) *
                           static_cast< std::int64_t >( byteSize ) );
}

void BM_SmallBlockBatchAllocateFree( benchmark::State& state )
{
  const Blue::Size byteSize = static_cast< Blue::Size >( state.range( 0 ) );

  MemorySystemScope memorySystem;
  if ( !PrepareMemorySystem( state, memorySystem ) || !VerifySmallBlockPath( state, byteSize ) )
  {
    return;
  }

  WarmUpSmallBlockAllocator( byteSize );

  void* pointers[ BatchSize ] = { };

  for ( auto _ : state )
  {
    for ( Blue::Size index = 0; index < BatchSize; ++index )
    {
      pointers[ index ] = Blue::AllocateSmallBlock( byteSize, SmallBlockAlignment );
      benchmark::DoNotOptimize( pointers[ index ] );
    }

    benchmark::ClobberMemory( );

    for ( Blue::Size index = 0; index < BatchSize; ++index )
    {
      Blue::FreeSmallBlock( pointers[ index ], byteSize, SmallBlockAlignment );
    }
  }

  state.SetItemsProcessed( static_cast< std::int64_t >( state.iterations( ) ) *
                           static_cast< std::int64_t >( BatchSize ) );

  state.SetBytesProcessed( static_cast< std::int64_t >( state.iterations( ) ) *
                           static_cast< std::int64_t >( BatchSize ) * static_cast< std::int64_t >( byteSize ) );
}
} // namespace


BENCHMARK( BM_RuntimeAllocateFree )
  ->Arg( 16 )
  ->Arg( 64 )
  ->Arg( 128 )
  ->Arg( 256 )
  ->Arg( 512 )
  ->Unit( benchmark::kNanosecond );

BENCHMARK( BM_StdOperatorNewDelete )
  ->Arg( 16 )
  ->Arg( 64 )
  ->Arg( 128 )
  ->Arg( 256 )
  ->Arg( 512 )
  ->Unit( benchmark::kNanosecond );

BENCHMARK( BM_BlueTryNewDeleteSmallObject )->Unit( benchmark::kNanosecond );

BENCHMARK( BM_StdNewDeleteSmallObject )->Unit( benchmark::kNanosecond );

BENCHMARK( BM_BlueTryNewDeleteMediumObject )->Unit( benchmark::kNanosecond );

BENCHMARK( BM_StdNewDeleteMediumObject )->Unit( benchmark::kNanosecond );

BENCHMARK( BM_SmallBlockAllocateFree )
  ->Arg( 16 )
  ->Arg( 64 )
  ->Arg( 128 )
  ->Arg( 256 )
  ->Arg( 512 )
  ->Unit( benchmark::kNanosecond );

BENCHMARK( BM_SmallBlockBatchAllocateFree )->Arg( 64 )->Arg( 128 )->Arg( 256 )->Unit( benchmark::kNanosecond );

BENCHMARK_MAIN( );

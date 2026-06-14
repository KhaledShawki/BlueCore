#include <Blue/Memory/Allocator/SmallBlockAllocator.h>
#include <Blue/Memory/Backend/SystemMemoryBackend.h>
#include <Blue/System/Alignment.h>
#include <Blue/System/Threading/Atomic.h>

namespace Blue
{
namespace
{
constexpr Size SmallBlockClassSizes[ BlueSmallBlockClassCount ] = { 16, 32, 64, 128, 256, 512 };
constexpr Size MaxSmallBlockSlabs = 4096;
constexpr Uint32 InvalidSmallBlockClass = 0xFFFFFFFFu;

struct SmallBlockSlab
{
	void* Base = nullptr;
	Size ClassSize = 0;
};

struct SmallBlockThreadCache
{
	void* FreeLists[ BlueSmallBlockClassCount ] = { };
	Uint32 Generation = 0;
};

SmallBlockSlab s_Slabs[ MaxSmallBlockSlabs ] = { };
AtomicUint32 s_Initialized( 0 );
AtomicUint32 s_Generation( 1 );
AtomicUint32 s_SlabCount( 0 );
AtomicUint64 s_AllocateCount( 0 );
AtomicUint64 s_FreeCount( 0 );
AtomicUint64 s_RefillCount( 0 );
AtomicUint64 s_FailedRefillCount( 0 );

thread_local SmallBlockThreadCache t_Cache = { };

BLUE_FORCE_INLINE Uint32 GetSmallBlockClassIndex( Size size, Size alignment ) noexcept
{
	const Size required = size > alignment ? size : alignment;
	if ( required == 0 || required > BlueSmallBlockMaxSize )
	{
		return InvalidSmallBlockClass;
	}

	for ( Uint32 index = 0; index < BlueSmallBlockClassCount; ++index )
	{
		if ( required <= SmallBlockClassSizes[ index ] )
		{
			return index;
		}
	}

	return InvalidSmallBlockClass;
}

BLUE_FORCE_INLINE void ResetThreadCacheIfNeeded( ) noexcept
{
	const Uint32 generation = s_Generation.Load( MemoryOrder::Acquire );
	if ( t_Cache.Generation == generation )
	{
		return;
	}

	for ( Size index = 0; index < BlueSmallBlockClassCount; ++index )
	{
		t_Cache.FreeLists[ index ] = nullptr;
	}
	t_Cache.Generation = generation;
}

BLUE_FORCE_INLINE void PushBlock( Uint32 classIndex, void* pointer ) noexcept
{
	*static_cast< void** >( pointer ) = t_Cache.FreeLists[ classIndex ];
	t_Cache.FreeLists[ classIndex ] = pointer;
}

BLUE_FORCE_INLINE void* PopBlock( Uint32 classIndex ) noexcept
{
	void* pointer = t_Cache.FreeLists[ classIndex ];
	if ( pointer )
	{
		t_Cache.FreeLists[ classIndex ] = *static_cast< void** >( pointer );
	}
	return pointer;
}

Bool RegisterSlab( void* base, Size classSize ) noexcept
{
	const Uint32 index = s_SlabCount.FetchAdd( 1, MemoryOrder::AcquireRelease );
	if ( index >= MaxSmallBlockSlabs )
	{
		s_SlabCount.FetchSub( 1, MemoryOrder::AcquireRelease );
		return false;
	}

	s_Slabs[ index ].Base = base;
	s_Slabs[ index ].ClassSize = classSize;
	return true;
}

Bool RefillThreadCache( Uint32 classIndex ) noexcept
{
	const Size classSize = SmallBlockClassSizes[ classIndex ];
	void* slab = SystemMemoryBackend::Allocate( BlueSmallBlockSlabSize, classSize );
	if ( !slab )
	{
		s_FailedRefillCount.FetchAdd( 1, MemoryOrder::Relaxed );
		return false;
	}

	if ( !RegisterSlab( slab, classSize ) )
	{
		SystemMemoryBackend::Free( slab, BlueSmallBlockSlabSize, classSize );
		s_FailedRefillCount.FetchAdd( 1, MemoryOrder::Relaxed );
		return false;
	}

	const Size blockCount = BlueSmallBlockSlabSize / classSize;
	Byte* cursor = static_cast< Byte* >( slab );
	for ( Size index = 0; index < blockCount; ++index )
	{
		PushBlock( classIndex, cursor + index * classSize );
	}

	s_RefillCount.FetchAdd( 1, MemoryOrder::Relaxed );
	return true;
}
} // namespace

Bool InitializeSmallBlockAllocator( ) noexcept
{
	ShutdownSmallBlockAllocator( );
	s_Initialized.Store( 1, MemoryOrder::Release );
	return true;
}

void ShutdownSmallBlockAllocator( ) noexcept
{
	const Uint32 slabCount = s_SlabCount.Load( MemoryOrder::Acquire );
	for ( Uint32 index = 0; index < slabCount && index < MaxSmallBlockSlabs; ++index )
	{
		if ( s_Slabs[ index ].Base )
		{
			SystemMemoryBackend::Free( s_Slabs[ index ].Base, BlueSmallBlockSlabSize, s_Slabs[ index ].ClassSize );
			s_Slabs[ index ] = { };
		}
	}

	s_SlabCount.Store( 0, MemoryOrder::Release );
	s_AllocateCount.Store( 0, MemoryOrder::Release );
	s_FreeCount.Store( 0, MemoryOrder::Release );
	s_RefillCount.Store( 0, MemoryOrder::Release );
	s_FailedRefillCount.Store( 0, MemoryOrder::Release );
	s_Initialized.Store( 0, MemoryOrder::Release );
	s_Generation.FetchAdd( 1, MemoryOrder::AcquireRelease );
}

Bool IsSmallBlockAllocationSupported( Size size, Size alignment ) noexcept
{
	if ( alignment == 0 || !IsPowerOfTwo( alignment ) )
	{
		return false;
	}

	return GetSmallBlockClassIndex( size, alignment ) != InvalidSmallBlockClass;
}

Size GetSmallBlockClassSize( Size size, Size alignment ) noexcept
{
	const Uint32 classIndex = GetSmallBlockClassIndex( size, alignment );
	return classIndex != InvalidSmallBlockClass ? SmallBlockClassSizes[ classIndex ] : 0;
}

void* AllocateSmallBlock( Size size, Size alignment ) noexcept
{
	if ( s_Initialized.Load( MemoryOrder::Acquire ) == 0 )
	{
		return nullptr;
	}

	const Uint32 classIndex = GetSmallBlockClassIndex( size, alignment );
	if ( classIndex == InvalidSmallBlockClass || !IsPowerOfTwo( alignment ) )
	{
		return nullptr;
	}

	ResetThreadCacheIfNeeded( );

	void* pointer = PopBlock( classIndex );
	if ( !pointer )
	{
		if ( !RefillThreadCache( classIndex ) )
		{
			return nullptr;
		}
		pointer = PopBlock( classIndex );
	}

	s_AllocateCount.FetchAdd( 1, MemoryOrder::Relaxed );
	return pointer;
}

void FreeSmallBlock( void* pointer, Size size, Size alignment ) noexcept
{
	if ( !pointer )
	{
		return;
	}

	const Uint32 classIndex = GetSmallBlockClassIndex( size, alignment );
	if ( classIndex == InvalidSmallBlockClass || !IsPowerOfTwo( alignment ) )
	{
		SystemMemoryBackend::Free( pointer, size, alignment );
		return;
	}

	ResetThreadCacheIfNeeded( );
	PushBlock( classIndex, pointer );
	s_FreeCount.FetchAdd( 1, MemoryOrder::Relaxed );
}

SmallBlockAllocatorStats GetSmallBlockAllocatorStats( ) noexcept
{
	SmallBlockAllocatorStats stats = { };
	stats.SlabCount = s_SlabCount.Load( MemoryOrder::Acquire );
	stats.SlabBytes = stats.SlabCount * BlueSmallBlockSlabSize;
	stats.AllocateCount = s_AllocateCount.Load( MemoryOrder::Acquire );
	stats.FreeCount = s_FreeCount.Load( MemoryOrder::Acquire );
	stats.RefillCount = s_RefillCount.Load( MemoryOrder::Acquire );
	stats.FailedRefillCount = s_FailedRefillCount.Load( MemoryOrder::Acquire );
	return stats;
}
} // namespace Blue

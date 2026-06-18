#include <Blue/Memory/VirtualMemoryBlock.h>
#include <Blue/System/Platform.h>

#if BLUE_PLATFORM == BLUE_PLATFORM_WINDOWS
#  include <windows.h>
#else
#  include <sys/mman.h>
#  include <unistd.h>
#endif

namespace Blue
{
static Size GetSystemPageSize( )
{
#if BLUE_PLATFORM == BLUE_PLATFORM_WINDOWS
  SYSTEM_INFO info;
  GetSystemInfo( &info );
  return static_cast< Size >( info.dwPageSize );
#else
  return static_cast< Size >( sysconf( _SC_PAGESIZE ) );
#endif
}

bool ReserveVirtualMemory( VirtualMemoryBlock& block, Size size )
{
  block = { };
  block.PageSize = GetSystemPageSize( );

#if BLUE_PLATFORM == BLUE_PLATFORM_WINDOWS
  void* memory = VirtualAlloc( nullptr, size, MEM_RESERVE, PAGE_READWRITE );
  if ( !memory )
  {
    return false;
  }
  block.ReservedBase = memory;
  block.ReservedSize = size;
  return true;
#else
  void* memory = mmap( nullptr, size, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0 );
  if ( memory == MAP_FAILED )
  {
    return false;
  }
  block.ReservedBase = memory;
  block.ReservedSize = size;
  return true;
#endif
}

bool CommitVirtualMemory( VirtualMemoryBlock& block, Size offset, Size size )
{
  if ( !block.ReservedBase || offset + size > block.ReservedSize )
  {
    return false;
  }

  void* address = static_cast< Byte* >( block.ReservedBase ) + offset;
#if BLUE_PLATFORM == BLUE_PLATFORM_WINDOWS
  void* result = VirtualAlloc( address, size, MEM_COMMIT, PAGE_READWRITE );
  if ( !result )
  {
    return false;
  }
#else
  if ( mprotect( address, size, PROT_READ | PROT_WRITE ) != 0 )
  {
    return false;
  }
#endif
  block.CommittedBase = block.CommittedBase ? block.CommittedBase : address;
  block.CommittedSize += size;
  return true;
}

bool DecommitVirtualMemory( VirtualMemoryBlock& block, Size offset, Size size )
{
  if ( !block.ReservedBase || offset + size > block.ReservedSize )
  {
    return false;
  }

  void* address = static_cast< Byte* >( block.ReservedBase ) + offset;
#if BLUE_PLATFORM == BLUE_PLATFORM_WINDOWS
  if ( !VirtualFree( address, size, MEM_DECOMMIT ) )
  {
    return false;
  }
#else
  if ( mprotect( address, size, PROT_NONE ) != 0 )
  {
    return false;
  }
#endif
  if ( block.CommittedSize >= size )
  {
    block.CommittedSize -= size;
  }
  return true;
}

void ReleaseVirtualMemory( VirtualMemoryBlock& block )
{
  if ( !block.ReservedBase )
  {
    return;
  }

#if BLUE_PLATFORM == BLUE_PLATFORM_WINDOWS
  VirtualFree( block.ReservedBase, 0, MEM_RELEASE );
#else
  munmap( block.ReservedBase, block.ReservedSize );
#endif
  block = { };
}
} // namespace Blue

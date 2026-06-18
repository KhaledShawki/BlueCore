#include <Blue/System/Threading/Processor.h>

namespace Blue
{
ProcessorArchitecture GetProcessorArchitecture( ) noexcept
{
#if BLUE_ARCH == BLUE_ARCH_X86
  return ProcessorArchitecture::X86;
#elif BLUE_ARCH == BLUE_ARCH_X64
  return ProcessorArchitecture::X64;
#elif BLUE_ARCH == BLUE_ARCH_ARM
  return ProcessorArchitecture::Arm;
#elif BLUE_ARCH == BLUE_ARCH_ARM64
  return ProcessorArchitecture::Arm64;
#else
  return ProcessorArchitecture::Unknown;
#endif
}

const Char* GetProcessorArchitectureName( ProcessorArchitecture architecture ) noexcept
{
  switch ( architecture )
  {
    case ProcessorArchitecture::X86 : return "x86";

    case ProcessorArchitecture::X64 : return "x64";

    case ProcessorArchitecture::Arm : return "ARM";

    case ProcessorArchitecture::Arm64 : return "ARM64";

    case ProcessorArchitecture::Unknown :
    default :                             return "Unknown";
  }
}

Uint32 GetRecommendedWorkerThreadCount( Uint32 reservedThreadCount ) noexcept
{
  const Uint32 logicalProcessorCount = GetLogicalProcessorCount( );

  if ( logicalProcessorCount <= 1 )
  {
    return 1;
  }

  if ( reservedThreadCount >= logicalProcessorCount )
  {
    return 1;
  }

  return logicalProcessorCount - reservedThreadCount;
}

ProcessorInfo QueryProcessorInfo( ) noexcept
{
  const ProcessorArchitecture architecture = GetProcessorArchitecture( );

  ProcessorInfo info;
  info.Architecture = architecture;
  info.LogicalProcessorCount = GetLogicalProcessorCount( );
  info.CacheLineSize = GetCacheLineSize( );
  info.ArchitectureName = GetProcessorArchitectureName( architecture );

  if ( info.LogicalProcessorCount == 0 )
  {
    info.LogicalProcessorCount = 1;
  }

  if ( info.CacheLineSize == 0 )
  {
    info.CacheLineSize = DefaultCacheLineSize;
  }

  return info;
}
} // namespace Blue

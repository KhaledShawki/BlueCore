#pragma once

#include <Blue/System/Api.h>
#include <Blue/System/Architecture.h>
#include <Blue/System/Compiler.h>
#include <Blue/System/Types.h>

namespace Blue
{
inline constexpr Uint32 DefaultCacheLineSize = 64;

enum class ProcessorArchitecture : Uint8
{
	Unknown,
	X86,
	X64,
	Arm,
	Arm64,
};

struct ProcessorInfo final
{
	ProcessorArchitecture Architecture;
	Uint32 LogicalProcessorCount;
	Uint32 CacheLineSize;
	const Char* ArchitectureName;
};

BLUE_SYSTEM_API ProcessorArchitecture GetProcessorArchitecture( ) noexcept;
BLUE_SYSTEM_API const Char* GetProcessorArchitectureName( ProcessorArchitecture architecture ) noexcept;

BLUE_SYSTEM_API Uint32 GetLogicalProcessorCount( ) noexcept;
BLUE_SYSTEM_API Uint32 GetCacheLineSize( ) noexcept;
BLUE_SYSTEM_API Uint32 GetRecommendedWorkerThreadCount( Uint32 reservedThreadCount = 1 ) noexcept;

BLUE_SYSTEM_API ProcessorInfo QueryProcessorInfo( ) noexcept;

BLUE_SYSTEM_API void YieldThread( );
BLUE_SYSTEM_API void SleepCurrentThread( Uint32 milliseconds );

BLUE_FORCE_INLINE void ProcessorPause( ) noexcept;
} // namespace Blue

#include <Blue/System/Threading/Processor.inl>

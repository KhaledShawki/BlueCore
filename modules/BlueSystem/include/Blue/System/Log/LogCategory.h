#pragma once

#include <Blue/System/Log/LogLevel.h>
#include <Blue/System/Types.h>

namespace Blue
{
struct LogCategory final
{
	const Char* Name = nullptr;
	LogLevel MinimumLevel = LogLevel::Info;
	Bool Enabled = true;
};
} // namespace Blue

#define BLUE_DECLARE_LOG_CATEGORY( categoryName ) extern Blue::LogCategory categoryName

#define BLUE_DEFINE_LOG_CATEGORY( categoryName, minimumLevel )                                                         \
	Blue::LogCategory categoryName                                                                                     \
	{                                                                                                                  \
		#categoryName, minimumLevel, true                                                                              \
	}

#define BLUE_DEFINE_INLINE_LOG_CATEGORY( categoryName, minimumLevel )                                                  \
	inline Blue::LogCategory categoryName                                                                              \
	{                                                                                                                  \
		#categoryName, minimumLevel, true                                                                              \
	}

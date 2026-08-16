#pragma once

#include <filesystem>
#include <string>
#include <vector>

#include "RunnerTypes.h"

namespace BlueTestRunner
{
void ConfigureChildTestEnvironment( );
// Arguments are UTF-8 strings. The executable path uses the platform-native filesystem representation.
ProcessResult RunProcess( const std::filesystem::path& executablePath,
                          const std::vector< std::string >& arguments,
                          const ProcessOptions& options );
} // namespace BlueTestRunner

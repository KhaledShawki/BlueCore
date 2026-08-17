#pragma once

#include <cstddef>
#include <filesystem>
#include <vector>

#include "RunnerTypes.h"

namespace BlueTestRunner
{
std::size_t ResolveWorkerCount( const RunnerOptions& options, std::size_t testCount );
RunSummary RunTests( const std::vector< std::filesystem::path >& testExecutables,
                     std::size_t workerCount,
                     const RunnerOptions& options );
std::uint32_t CountUnsuccessfulTests( const std::vector< TestResult >& results );
} // namespace BlueTestRunner

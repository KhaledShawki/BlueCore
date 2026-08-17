#pragma once

#include <cstddef>
#include <filesystem>
#include <vector>

#include "RunnerTypes.h"

namespace BlueTestRunner
{
void PrintHeader( const RunnerOptions& options, std::size_t total, std::size_t workerCount );
void PrintTestList( const std::vector< std::filesystem::path >& testExecutables );
void PrintResults( const std::vector< TestResult >& results );
void PrintSummary( const RunSummary& summary );
} // namespace BlueTestRunner

#pragma once

#include <span>
#include <string_view>

#include "RunnerTypes.h"

namespace BlueTestRunner
{
ParsedArguments ParseArguments( int argc, char** argv );
ParsedArguments ParseArguments( std::span< const std::string_view > arguments );
void PrintUsage( );
} // namespace BlueTestRunner

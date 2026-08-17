#include "RunnerOptions.h"

#include <charconv>
#include <iostream>
#include <limits>
#include <string>
#include <vector>

namespace BlueTestRunner
{
namespace
{
constexpr std::uint64_t MaximumTimeoutMilliseconds = 24ULL * 60ULL * 60ULL * 1000ULL;
constexpr std::uint64_t MaximumOutputBytes = 1024ULL * 1024ULL * 1024ULL;

bool StartsWith( const std::string_view value, const std::string_view prefix )
{
  return value.size( ) >= prefix.size( ) && value.substr( 0, prefix.size( ) ) == prefix;
}

bool TryParsePositiveInteger( const std::string_view value, std::uint64_t& result )
{
  if ( value.empty( ) )
  {
    return false;
  }

  std::uint64_t parsed = 0;
  const char* const begin = value.data( );
  const char* const end = value.data( ) + value.size( );
  const std::from_chars_result parsedResult = std::from_chars( begin, end, parsed, 10 );

  if ( parsedResult.ec != std::errc{ } || parsedResult.ptr != end || parsed == 0 )
  {
    return false;
  }

  result = parsed;
  return true;
}

bool TryScaleValue( const std::uint64_t value, const std::uint64_t multiplier, std::uint64_t& result )
{
  if ( value > std::numeric_limits< std::uint64_t >::max( ) / multiplier )
  {
    return false;
  }

  result = value * multiplier;
  return true;
}

bool TryParseDuration( const std::string_view value, std::chrono::milliseconds& result )
{
  if ( value.empty( ) )
  {
    return false;
  }

  std::string_view numericPart = value;
  std::uint64_t multiplier = 0;

  if ( value.ends_with( "ms" ) )
  {
    numericPart.remove_suffix( 2 );
    multiplier = 1;
  }
  else if ( value.ends_with( "s" ) )
  {
    numericPart.remove_suffix( 1 );
    multiplier = 1000;
  }
  else if ( value.ends_with( "m" ) )
  {
    numericPart.remove_suffix( 1 );
    multiplier = 60U * 1000U;
  }
  else
  {
    return false;
  }

  std::uint64_t amount = 0;
  std::uint64_t milliseconds = 0;
  if ( !TryParsePositiveInteger( numericPart, amount ) || !TryScaleValue( amount, multiplier, milliseconds ) )
  {
    return false;
  }

  if ( milliseconds > MaximumTimeoutMilliseconds )
  {
    return false;
  }

  result = std::chrono::milliseconds{ static_cast< std::int64_t >( milliseconds ) };
  return true;
}

bool TryParseByteSize( const std::string_view value, std::size_t& result )
{
  if ( value.empty( ) )
  {
    return false;
  }

  std::string_view numericPart = value;
  std::uint64_t multiplier = 1;

  if ( value.ends_with( "k" ) || value.ends_with( "K" ) )
  {
    numericPart.remove_suffix( 1 );
    multiplier = 1024U;
  }
  else if ( value.ends_with( "m" ) || value.ends_with( "M" ) )
  {
    numericPart.remove_suffix( 1 );
    multiplier = 1024U * 1024U;
  }
  else if ( value.ends_with( "g" ) || value.ends_with( "G" ) )
  {
    numericPart.remove_suffix( 1 );
    multiplier = 1024ULL * 1024ULL * 1024ULL;
  }
  else if ( value.ends_with( "b" ) || value.ends_with( "B" ) )
  {
    numericPart.remove_suffix( 1 );
  }

  std::uint64_t amount = 0;
  std::uint64_t bytes = 0;
  if ( !TryParsePositiveInteger( numericPart, amount ) || !TryScaleValue( amount, multiplier, bytes ) )
  {
    return false;
  }

  if ( bytes > MaximumOutputBytes ||
       bytes > static_cast< std::uint64_t >( std::numeric_limits< std::size_t >::max( ) ) )
  {
    return false;
  }

  result = static_cast< std::size_t >( bytes );
  return true;
}

ParsedArguments ParseArgumentsImpl( const std::span< const std::string_view > arguments )
{
  ParsedArguments parsed;
  bool parseOptions = true;

  for ( const std::string_view argument : arguments )
  {
    if ( parseOptions && argument == "--" )
    {
      parseOptions = false;
      continue;
    }

    if ( parseOptions && ( argument == "--help" || argument == "-h" ) )
    {
      parsed.options.help = true;
      return parsed;
    }

    if ( parseOptions && argument == "--list" )
    {
      parsed.options.listOnly = true;
      continue;
    }

    if ( parseOptions && argument == "--sequential" )
    {
      parsed.options.mode = JobMode::Sequential;
      parsed.options.requestedJobCount = 1;
      continue;
    }

    if ( parseOptions && StartsWith( argument, "--jobs=" ) )
    {
      const std::string_view value = argument.substr( 7 );
      if ( value == "auto" )
      {
        parsed.options.mode = JobMode::Auto;
        parsed.options.requestedJobCount = 0;
        continue;
      }

      std::uint64_t jobCount = 0;
      if ( !TryParsePositiveInteger( value, jobCount ) || jobCount > std::numeric_limits< std::size_t >::max( ) )
      {
        parsed.success = false;
        parsed.exitCode = 1;
        parsed.errorMessage = "Invalid --jobs value: " + std::string( value );
        return parsed;
      }

      parsed.options.mode = JobMode::Fixed;
      parsed.options.requestedJobCount = static_cast< std::size_t >( jobCount );
      continue;
    }

    if ( parseOptions && StartsWith( argument, "--timeout=" ) )
    {
      const std::string_view value = argument.substr( 10 );
      if ( !TryParseDuration( value, parsed.options.timeout ) )
      {
        parsed.success = false;
        parsed.exitCode = 1;
        parsed.errorMessage = "Invalid --timeout value: " + std::string( value );
        return parsed;
      }
      continue;
    }

    if ( parseOptions && StartsWith( argument, "--max-output=" ) )
    {
      const std::string_view value = argument.substr( 13 );
      if ( !TryParseByteSize( value, parsed.options.maximumOutputBytes ) )
      {
        parsed.success = false;
        parsed.exitCode = 1;
        parsed.errorMessage = "Invalid --max-output value: " + std::string( value );
        return parsed;
      }
      continue;
    }

    if ( parseOptions && StartsWith( argument, "--" ) )
    {
      parsed.success = false;
      parsed.exitCode = 1;
      parsed.errorMessage = "Unknown option: " + std::string( argument );
      return parsed;
    }

    parsed.testExecutables.emplace_back( argument );
  }

  if ( parsed.testExecutables.empty( ) && !parsed.options.help )
  {
    parsed.success = false;
    parsed.exitCode = 1;
    parsed.errorMessage = "No test executables were provided.";
  }

  return parsed;
}
} // namespace

ParsedArguments ParseArguments( const int argc, char** argv )
{
  std::vector< std::string_view > arguments;
  arguments.reserve( argc > 1 ? static_cast< std::size_t >( argc - 1 ) : 0 );

  for ( int index = 1; index < argc; ++index )
  {
    arguments.emplace_back( argv[ index ] );
  }

  return ParseArgumentsImpl( arguments );
}

ParsedArguments ParseArguments( const std::span< const std::string_view > arguments )
{
  return ParseArgumentsImpl( arguments );
}

void PrintUsage( )
{
  std::cout << "BlueRunTests\n\n";
  std::cout << "Usage:\n";
  std::cout << "  BlueRunTests [options] <test-executable>...\n\n";
  std::cout << "Options:\n";
  std::cout << "  --jobs=auto          Run test executables using an automatic worker count.\n";
  std::cout << "  --jobs=N             Run up to N test executables in parallel.\n";
  std::cout << "  --sequential         Run test executables sequentially.\n";
  std::cout << "  --timeout=DURATION   Per-executable timeout (examples: 500ms, 30s, 2m; default: 120s, max: 1440m).\n";
  std::cout << "  --max-output=SIZE    Maximum captured output payload (examples: 64k, 8m; default: 8m, max: 1g).\n";
  std::cout << "  --list               List test executables without running them.\n";
  std::cout << "  --help, -h           Show this help text.\n";
  std::cout << "  --                   Treat all following arguments as test executable paths.\n";
}
} // namespace BlueTestRunner

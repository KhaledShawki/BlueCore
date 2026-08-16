#include "TestReporter.h"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <string_view>
#include <vector>

namespace BlueTestRunner
{
namespace
{
constexpr double SlowExecutableWarningMilliseconds = 1000.0;
constexpr std::size_t SlowExecutableReportCount = 5;

struct StatusCounts
{
  std::uint32_t passed = 0;
  std::uint32_t failed = 0;
  std::uint32_t timedOut = 0;
  std::uint32_t missing = 0;
  std::uint32_t launchFailed = 0;
  std::uint32_t runnerError = 0;
};

void PrintLine( )
{
  std::cout << "------------------------------------------------------------\n";
}

const char* ExecutionModeName( const std::size_t workerCount )
{
  return workerCount <= 1 ? "sequential" : "parallel";
}

const char* StatusName( const TestStatus status )
{
  switch ( status )
  {
    case TestStatus::Passed :       return "PASS";
    case TestStatus::Failed :       return "FAIL";
    case TestStatus::TimedOut :     return "TIMEOUT";
    case TestStatus::Missing :      return "MISSING";
    case TestStatus::LaunchFailed : return "LAUNCH_FAILED";
    case TestStatus::RunnerError :  return "RUNNER_ERROR";
  }

  return "RUNNER_ERROR";
}

void PrintCapturedOutput( const std::string& output )
{
  if ( output.empty( ) )
  {
    return;
  }

  std::cout << output;
  if ( output.back( ) != '\n' )
  {
    std::cout << '\n';
  }
}

StatusCounts CountStatuses( const std::vector< TestResult >& results )
{
  StatusCounts counts;
  for ( const TestResult& result : results )
  {
    switch ( result.status )
    {
      case TestStatus::Passed :       ++counts.passed; break;
      case TestStatus::Failed :       ++counts.failed; break;
      case TestStatus::TimedOut :     ++counts.timedOut; break;
      case TestStatus::Missing :      ++counts.missing; break;
      case TestStatus::LaunchFailed : ++counts.launchFailed; break;
      case TestStatus::RunnerError :  ++counts.runnerError; break;
    }
  }
  return counts;
}

double SumElapsedDurations( const std::vector< TestResult >& results )
{
  double totalMilliseconds = 0.0;
  for ( const TestResult& result : results )
  {
    totalMilliseconds += result.elapsedMilliseconds;
  }
  return totalMilliseconds;
}

void PrintSlowestExecutables( const std::vector< TestResult >& results )
{
  if ( results.empty( ) )
  {
    return;
  }

  std::vector< std::size_t > indices( results.size( ) );
  for ( std::size_t index = 0; index < indices.size( ); ++index )
  {
    indices[ index ] = index;
  }

  std::sort( indices.begin( ),
             indices.end( ),
             [ &results ]( const std::size_t left, const std::size_t right )
             {
               return results[ left ].elapsedMilliseconds > results[ right ].elapsedMilliseconds;
             } );

  std::cout << "  Slowest     :\n";
  const std::size_t count = std::min( SlowExecutableReportCount, indices.size( ) );
  for ( std::size_t rank = 0; rank < count; ++rank )
  {
    const TestResult& result = results[ indices[ rank ] ];
    std::cout << "    " << rank + 1 << ". " << result.executablePath.filename( ).string( )
              << " elapsed_ms=" << result.elapsedMilliseconds << "\n";
  }
}
} // namespace

void PrintHeader( const RunnerOptions& options, const std::size_t total, const std::size_t workerCount )
{
  std::cout << "============================================================\n";
  std::cout << "Blue Test Runner\n";
  std::cout << "============================================================\n";
  std::cout << "Registered test executables: " << total << "\n";
  std::cout << "Execution mode             : " << ExecutionModeName( workerCount ) << "\n";
  std::cout << "Workers                    : " << workerCount << "\n";
  std::cout << "Timeout ms                 : " << options.timeout.count( ) << "\n";
  std::cout << "Maximum output bytes       : " << options.maximumOutputBytes << "\n";
}

void PrintTestList( const std::vector< std::filesystem::path >& testExecutables )
{
  PrintLine( );
  std::cout << "Registered test executables:\n";
  for ( std::size_t index = 0; index < testExecutables.size( ); ++index )
  {
    std::cout << "  " << index + 1 << ". " << testExecutables[ index ].string( ) << "\n";
  }
  PrintLine( );
}

void PrintResults( const std::vector< TestResult >& results )
{
  for ( std::size_t index = 0; index < results.size( ); ++index )
  {
    const TestResult& result = results[ index ];
    PrintLine( );
    std::cout << "[BlueRunTests] Test " << index + 1 << '/' << results.size( ) << "\n";
    std::cout << "[BlueRunTests] Executable: " << result.executablePath.string( ) << "\n";
    PrintCapturedOutput( result.output );

    std::ostream& stream = result.status == TestStatus::Passed ? std::cout : std::cerr;
    stream << "[BlueRunTests] Result: " << StatusName( result.status );
    if ( result.status == TestStatus::Failed )
    {
      stream << " exit=" << result.exitCode;
    }
    stream << " elapsed_ms=" << result.elapsedMilliseconds << "\n";

    if ( !result.errorMessage.empty( ) )
    {
      std::cerr << "[BlueRunTests] Error: " << result.errorMessage << "\n";
    }

    if ( result.outputTruncated )
    {
      std::cout << "[BlueRunTests] Output bytes produced=" << result.producedOutputBytes
                << " captured_payload_limit_reached=true\n";
    }

    if ( result.elapsedMilliseconds >= SlowExecutableWarningMilliseconds )
    {
      std::cout << "[BlueRunTests] Warning: slow test executable elapsed_ms=" << result.elapsedMilliseconds
                << " executable=" << result.executablePath.string( ) << "\n";
    }
  }
}

void PrintSummary( const RunSummary& summary )
{
  const StatusCounts counts = CountStatuses( summary.results );

  PrintLine( );
  std::cout << "Blue Test Runner Summary\n";
  std::cout << "  Total       : " << summary.results.size( ) << "\n";
  std::cout << "  Passed      : " << counts.passed << "\n";
  std::cout << "  Failed      : " << counts.failed << "\n";
  std::cout << "  Timed out   : " << counts.timedOut << "\n";
  std::cout << "  Missing     : " << counts.missing << "\n";
  std::cout << "  Launch fail : " << counts.launchFailed << "\n";
  std::cout << "  Runner error: " << counts.runnerError << "\n";
  std::cout << "  Mode        : " << ExecutionModeName( summary.workerCount ) << "\n";
  std::cout << "  Workers     : " << summary.workerCount << "\n";
  std::cout << "  Wall elapsed ms: " << summary.wallElapsedMilliseconds << "\n";
  std::cout << "  Sum elapsed ms: " << SumElapsedDurations( summary.results ) << "\n";
  PrintSlowestExecutables( summary.results );
  PrintLine( );

  if ( counts.failed == 0 && counts.timedOut == 0 && counts.missing == 0 && counts.launchFailed == 0 &&
       counts.runnerError == 0 )
  {
    std::cout << "All registered Blue test executables passed.\n";
  }
  else
  {
    std::cerr << "One or more Blue test executables did not pass.\n";
  }
}
} // namespace BlueTestRunner

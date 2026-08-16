#include "TestScheduler.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <system_error>
#include <thread>

#include "ProcessRunner.h"

namespace BlueTestRunner
{
namespace
{
constexpr std::size_t DefaultMaximumAutoJobCount = 8;

TestStatus ResolveTestStatus( const ProcessResult& processResult )
{
  switch ( processResult.status )
  {
    case ProcessStatus::Exited :       return processResult.exitCode == 0 ? TestStatus::Passed : TestStatus::Failed;
    case ProcessStatus::TimedOut :     return TestStatus::TimedOut;
    case ProcessStatus::LaunchFailed : return TestStatus::LaunchFailed;
    case ProcessStatus::RunnerError :  return TestStatus::RunnerError;
  }

  return TestStatus::RunnerError;
}

TestResult RunTestExecutable( const std::filesystem::path& executablePath, const RunnerOptions& options )
{
  TestResult result;
  result.executablePath = executablePath;

  std::error_code error;
  const bool exists = std::filesystem::exists( executablePath, error );
  if ( error )
  {
    result.status = TestStatus::RunnerError;
    result.errorMessage = "Could not query test executable: " + error.message( );
    return result;
  }

  if ( !exists )
  {
    result.status = TestStatus::Missing;
    result.errorMessage = "Test executable does not exist.";
    return result;
  }

  const auto startTime = std::chrono::steady_clock::now( );
  const ProcessResult processResult = RunProcess( executablePath,
                                                  { },
                                                  ProcessOptions{
                                                    .timeout = options.timeout,
                                                    .maximumOutputBytes = options.maximumOutputBytes,
                                                  } );
  const auto endTime = std::chrono::steady_clock::now( );

  result.status = ResolveTestStatus( processResult );
  result.exitCode = processResult.exitCode;
  result.output = processResult.output;
  result.producedOutputBytes = processResult.producedOutputBytes;
  result.outputTruncated = processResult.outputTruncated;
  result.errorMessage = processResult.errorMessage;
  result.elapsedMilliseconds = std::chrono::duration< double, std::milli >( endTime - startTime ).count( );
  return result;
}

std::vector< TestResult > RunSequential( const std::vector< std::filesystem::path >& testExecutables,
                                         const RunnerOptions& options )
{
  std::vector< TestResult > results;
  results.reserve( testExecutables.size( ) );

  for ( const std::filesystem::path& executablePath : testExecutables )
  {
    results.push_back( RunTestExecutable( executablePath, options ) );
  }

  return results;
}

std::vector< TestResult > RunParallel( const std::vector< std::filesystem::path >& testExecutables,
                                       const std::size_t workerCount,
                                       const RunnerOptions& options )
{
  std::vector< TestResult > results( testExecutables.size( ) );
  std::atomic< std::size_t > nextIndex{ 0 };

  std::vector< std::thread > workers;
  workers.reserve( workerCount );

  for ( std::size_t workerIndex = 0; workerIndex < workerCount; ++workerIndex )
  {
    workers.emplace_back(
      [ &results, &testExecutables, &nextIndex, &options ]( )
      {
        for ( ;; )
        {
          const std::size_t testIndex = nextIndex.fetch_add( 1, std::memory_order_relaxed );
          if ( testIndex >= testExecutables.size( ) )
          {
            break;
          }

          results[ testIndex ] = RunTestExecutable( testExecutables[ testIndex ], options );
        }
      } );
  }

  for ( std::thread& worker : workers )
  {
    worker.join( );
  }

  return results;
}
} // namespace

std::size_t ResolveWorkerCount( const RunnerOptions& options, const std::size_t testCount )
{
  if ( testCount == 0 )
  {
    return 0;
  }

  if ( options.mode == JobMode::Sequential )
  {
    return 1;
  }

  if ( options.mode == JobMode::Fixed )
  {
    return std::max< std::size_t >( 1, std::min( options.requestedJobCount, testCount ) );
  }

  std::size_t hardwareThreadCount = std::thread::hardware_concurrency( );
  if ( hardwareThreadCount == 0 )
  {
    hardwareThreadCount = 2;
  }

  return std::max< std::size_t >( 1,
                                  std::min( testCount, std::min( hardwareThreadCount, DefaultMaximumAutoJobCount ) ) );
}

RunSummary RunTests( const std::vector< std::filesystem::path >& testExecutables,
                     const std::size_t workerCount,
                     const RunnerOptions& options )
{
  RunSummary summary;
  summary.workerCount = workerCount;

  const auto startTime = std::chrono::steady_clock::now( );
  if ( workerCount <= 1 || testExecutables.size( ) <= 1 )
  {
    summary.results = RunSequential( testExecutables, options );
  }
  else
  {
    summary.results = RunParallel( testExecutables, workerCount, options );
  }

  summary.wallElapsedMilliseconds =
    std::chrono::duration< double, std::milli >( std::chrono::steady_clock::now( ) - startTime ).count( );
  return summary;
}

std::uint32_t CountUnsuccessfulTests( const std::vector< TestResult >& results )
{
  std::uint32_t count = 0;
  for ( const TestResult& result : results )
  {
    if ( result.status != TestStatus::Passed )
    {
      ++count;
    }
  }
  return count;
}
} // namespace BlueTestRunner

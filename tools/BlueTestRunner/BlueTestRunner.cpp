#include <iostream>

#include "ProcessRunner.h"
#include "RunnerOptions.h"
#include "TestReporter.h"
#include "TestScheduler.h"

int main( const int argc, char** argv )
{
  using namespace BlueTestRunner;

  ConfigureChildTestEnvironment( );
  const ParsedArguments parsed = ParseArguments( argc, argv );

  if ( !parsed.success )
  {
    std::cerr << "[BlueRunTests] " << parsed.errorMessage << "\n";
    std::cerr << "Use --help for usage.\n";
    return parsed.exitCode;
  }

  if ( parsed.options.help )
  {
    PrintUsage( );
    return 0;
  }

  const std::size_t workerCount = ResolveWorkerCount( parsed.options, parsed.testExecutables.size( ) );
  PrintHeader( parsed.options, parsed.testExecutables.size( ), workerCount );

  if ( parsed.options.listOnly )
  {
    PrintTestList( parsed.testExecutables );
    return 0;
  }

  const RunSummary summary = RunTests( parsed.testExecutables, workerCount, parsed.options );
  PrintResults( summary.results );
  PrintSummary( summary );
  return CountUnsuccessfulTests( summary.results ) == 0 ? 0 : 1;
}

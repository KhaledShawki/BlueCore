#include <chrono>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

#if defined( _WIN32 )
#  include <Blue/System/Platform/WindowsLean.h>
#else
#  include <sys/wait.h>
#  include <unistd.h>
#endif

namespace
{
struct TestResult
{
  std::string Name;
  int ExitCode = 1;
  bool Started = false;
  double DurationMilliseconds = 0.0;
};

#if defined( _WIN32 )
std::string QuoteCommandArgument( const std::string& value )
{
  std::string result;
  result.reserve( value.size( ) + 2 );
  result.push_back( '"' );

  for ( char character : value )
  {
    if ( character == '"' )
    {
      result.push_back( '\\' );
    }

    result.push_back( character );
  }

  result.push_back( '"' );
  return result;
}

int RunProcess( const std::string& executablePath )
{
  std::string commandLine = QuoteCommandArgument( executablePath );

  STARTUPINFOA startupInfo{ };
  startupInfo.cb = sizeof( startupInfo );

  PROCESS_INFORMATION processInfo{ };

  BOOL created = CreateProcessA( nullptr,
                                 commandLine.data( ),
                                 nullptr,
                                 nullptr,
                                 FALSE,
                                 0,
                                 nullptr,
                                 nullptr,
                                 &startupInfo,
                                 &processInfo );

  if ( !created )
  {
    std::cerr << "[BlueRunTests] CreateProcess failed for: " << executablePath << " error=" << GetLastError( ) << '\n';
    return 1;
  }

  WaitForSingleObject( processInfo.hProcess, INFINITE );

  DWORD exitCode = 1;
  if ( !GetExitCodeProcess( processInfo.hProcess, &exitCode ) )
  {
    std::cerr << "[BlueRunTests] GetExitCodeProcess failed for: " << executablePath << " error=" << GetLastError( )
              << '\n';
    exitCode = 1;
  }

  CloseHandle( processInfo.hThread );
  CloseHandle( processInfo.hProcess );

  return static_cast< int >( exitCode );
}
#else
int RunProcess( const std::string& executablePath )
{
  const pid_t pid = fork( );

  if ( pid < 0 )
  {
    std::cerr << "[BlueRunTests] fork failed for: " << executablePath << '\n';
    return 1;
  }

  if ( pid == 0 )
  {
    execl( executablePath.c_str( ), executablePath.c_str( ), static_cast< char* >( nullptr ) );
    _exit( 127 );
  }

  int status = 0;
  if ( waitpid( pid, &status, 0 ) < 0 )
  {
    std::cerr << "[BlueRunTests] waitpid failed for: " << executablePath << '\n';
    return 1;
  }

  if ( WIFEXITED( status ) )
  {
    return WEXITSTATUS( status );
  }

  if ( WIFSIGNALED( status ) )
  {
    return 128 + WTERMSIG( status );
  }

  return 1;
}
#endif

void PrintLine( )
{
  std::cout << "------------------------------------------------------------\n";
}

void PrintHeader( int total )
{
  std::cout << "============================================================\n";
  std::cout << "Blue Test Runner\n";
  std::cout << "============================================================\n";
  std::cout << "Registered test executables: " << total << "\n";
}

TestResult RunTestExecutable( const std::string& executablePath, int index, int total )
{
  TestResult result;
  result.Name = executablePath;

  PrintLine( );
  std::cout << "[BlueRunTests] Test " << index << '/' << total << "\n";
  std::cout << "[BlueRunTests] Executable: " << executablePath << "\n";

  if ( !std::filesystem::exists( executablePath ) )
  {
    std::cerr << "[BlueRunTests] Result: MISSING\n";
    result.ExitCode = 1;
    return result;
  }

  result.Started = true;

  const auto startTime = std::chrono::steady_clock::now( );
  result.ExitCode = RunProcess( executablePath );
  const auto endTime = std::chrono::steady_clock::now( );

  const std::chrono::duration< double, std::milli > elapsed = endTime - startTime;
  result.DurationMilliseconds = elapsed.count( );

  if ( result.ExitCode == 0 )
  {
    std::cout << "[BlueRunTests] Result: PASS"
              << " duration_ms=" << result.DurationMilliseconds << "\n";
  }
  else
  {
    std::cerr << "[BlueRunTests] Result: FAIL"
              << " exit=" << result.ExitCode << " duration_ms=" << result.DurationMilliseconds << "\n";
  }

  return result;
}

std::uint32_t CountFailedTests( const std::vector< TestResult >& results )
{
  std::uint32_t failedCount = 0;
  for ( const TestResult& result : results )
  {
    if ( result.ExitCode != 0 )
    {
      ++failedCount;
    }
  }

  return failedCount;
}

double SumDurations( const std::vector< TestResult >& results )
{
  double totalMilliseconds = 0.0;
  for ( const TestResult& result : results )
  {
    totalMilliseconds += result.DurationMilliseconds;
  }

  return totalMilliseconds;
}

void PrintSummary( const std::vector< TestResult >& results )
{
  const std::uint32_t failedCount = CountFailedTests( results );
  const std::uint32_t passedCount = static_cast< std::uint32_t >( results.size( ) ) - failedCount;

  PrintLine( );
  std::cout << "Blue Test Runner Summary\n";
  std::cout << "  Total      : " << results.size( ) << "\n";
  std::cout << "  Passed     : " << passedCount << "\n";
  std::cout << "  Failed     : " << failedCount << "\n";
  std::cout << "  Duration ms: " << SumDurations( results ) << "\n";
  PrintLine( );

  if ( failedCount == 0 )
  {
    std::cout << "All registered Blue test executables passed.\n";
  }
  else
  {
    std::cerr << "One or more Blue test executables failed.\n";
  }
}
} // namespace

int main( int argc, char** argv )
{
  if ( argc <= 1 )
  {
    std::cerr << "[BlueRunTests] No test executables were provided.\n";
    return 1;
  }

  const int totalTests = argc - 1;
  PrintHeader( totalTests );

  std::vector< TestResult > results;
  results.reserve( static_cast< std::size_t >( totalTests ) );

  for ( int index = 1; index < argc; ++index )
  {
    results.push_back( RunTestExecutable( argv[ index ], index, totalTests ) );
  }

  PrintSummary( results );
  return CountFailedTests( results ) == 0 ? 0 : 1;
}

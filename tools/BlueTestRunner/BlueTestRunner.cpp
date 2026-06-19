#include <algorithm>
#include <atomic>
#include <cerrno>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <limits>
#include <string>
#include <thread>
#include <vector>

#if defined( _WIN32 )
#  include <Blue/System/Platform/WindowsLean.h>
#else
#  include <sys/wait.h>
#  include <unistd.h>
#endif

namespace
{
constexpr std::size_t DefaultMaximumAutoJobCount = 8;
constexpr double SlowExecutableWarningMilliseconds = 1000.0;
constexpr std::size_t SlowExecutableReportCount = 5;

enum class JobMode
{
  Sequential,
  Fixed,
  Auto,
};

struct RunnerOptions
{
  JobMode Mode = JobMode::Auto;
  std::size_t RequestedJobCount = 0;
  bool ListOnly = false;
  bool Help = false;
};

struct ParsedArguments
{
  RunnerOptions Options;
  std::vector< std::string > TestExecutables;
  bool Success = true;
  int ExitCode = 0;
  std::string ErrorMessage;
};

struct ProcessResult
{
  int ExitCode = 1;
  std::string Output;
};

struct TestResult
{
  std::string ExecutablePath;
  int ExitCode = 1;
  bool Started = false;
  bool Missing = false;
  double DurationMilliseconds = 0.0;
  std::string Output;
};

struct RunSummary
{
  std::vector< TestResult > Results;
  double WallDurationMilliseconds = 0.0;
  std::size_t WorkerCount = 1;
};

void ConfigureChildTestEnvironment( )
{
#if defined( _WIN32 )
  SetEnvironmentVariableA( "GTEST_COLOR", "yes" );
#else
  setenv( "GTEST_COLOR", "yes", 1 );
#endif
}

bool StartsWith( const std::string& value, const std::string& prefix )
{
  return value.size( ) >= prefix.size( ) && value.compare( 0, prefix.size( ), prefix ) == 0;
}

bool TryParsePositiveSize( const std::string& value, std::size_t& result )
{
  if ( value.empty( ) )
  {
    return false;
  }

  char* end = nullptr;
  errno = 0;

  const unsigned long long parsed = std::strtoull( value.c_str( ), &end, 10 );

  if ( errno != 0 || end == value.c_str( ) || *end != '\0' || parsed == 0 )
  {
    return false;
  }

  if ( parsed > static_cast< unsigned long long >( std::numeric_limits< std::size_t >::max( ) ) )
  {
    return false;
  }

  result = static_cast< std::size_t >( parsed );
  return true;
}

std::string GetFileName( const std::string& path )
{
  return std::filesystem::path( path ).filename( ).string( );
}

void PrintLine( )
{
  std::cout << "------------------------------------------------------------\n";
}

void PrintUsage( )
{
  std::cout << "BlueRunTests\n";
  std::cout << '\n';
  std::cout << "Usage:\n";
  std::cout << "  BlueRunTests [options] <test-executable>...\n";
  std::cout << '\n';
  std::cout << "Options:\n";
  std::cout << "  --jobs=auto       Run test executables in parallel using an automatic worker count.\n";
  std::cout << "  --jobs=N          Run up to N test executables in parallel.\n";
  std::cout << "  --sequential      Run test executables sequentially.\n";
  std::cout << "  --list            List test executables without running them.\n";
  std::cout << "  --help, -h        Show this help text.\n";
  std::cout << "  --                Treat all following arguments as test executable paths.\n";
}

ParsedArguments ParseArguments( int argc, char** argv )
{
  ParsedArguments parsed;
  bool parseOptions = true;

  for ( int index = 1; index < argc; ++index )
  {
    const std::string argument = argv[ index ];

    if ( parseOptions && argument == "--" )
    {
      parseOptions = false;
      continue;
    }

    if ( parseOptions && ( argument == "--help" || argument == "-h" ) )
    {
      parsed.Options.Help = true;
      return parsed;
    }

    if ( parseOptions && argument == "--list" )
    {
      parsed.Options.ListOnly = true;
      continue;
    }

    if ( parseOptions && argument == "--sequential" )
    {
      parsed.Options.Mode = JobMode::Sequential;
      parsed.Options.RequestedJobCount = 1;
      continue;
    }

    if ( parseOptions && StartsWith( argument, "--jobs=" ) )
    {
      const std::string value = argument.substr( 7 );

      if ( value == "auto" )
      {
        parsed.Options.Mode = JobMode::Auto;
        parsed.Options.RequestedJobCount = 0;
        continue;
      }

      std::size_t jobCount = 0;
      if ( !TryParsePositiveSize( value, jobCount ) )
      {
        parsed.Success = false;
        parsed.ExitCode = 1;
        parsed.ErrorMessage = "Invalid --jobs value: " + value;
        return parsed;
      }

      parsed.Options.Mode = JobMode::Fixed;
      parsed.Options.RequestedJobCount = jobCount;
      continue;
    }

    if ( parseOptions && StartsWith( argument, "--" ) )
    {
      parsed.Success = false;
      parsed.ExitCode = 1;
      parsed.ErrorMessage = "Unknown option: " + argument;
      return parsed;
    }

    parsed.TestExecutables.push_back( argument );
  }

  if ( parsed.TestExecutables.empty( ) && !parsed.Options.Help )
  {
    parsed.Success = false;
    parsed.ExitCode = 1;
    parsed.ErrorMessage = "No test executables were provided.";
  }

  return parsed;
}

std::size_t ResolveWorkerCount( const RunnerOptions& options, std::size_t testCount )
{
  if ( testCount == 0 )
  {
    return 0;
  }

  if ( options.Mode == JobMode::Sequential )
  {
    return 1;
  }

  if ( options.Mode == JobMode::Fixed )
  {
    return std::max< std::size_t >( 1, std::min( options.RequestedJobCount, testCount ) );
  }

  std::size_t hardwareThreadCount = std::thread::hardware_concurrency( );
  if ( hardwareThreadCount == 0 )
  {
    hardwareThreadCount = 2;
  }

  const std::size_t cappedHardwareThreadCount = std::min( hardwareThreadCount, DefaultMaximumAutoJobCount );
  return std::max< std::size_t >( 1, std::min( testCount, cappedHardwareThreadCount ) );
}

const char* GetExecutionModeName( std::size_t workerCount )
{
  return workerCount <= 1 ? "sequential" : "parallel";
}

void PrintHeader( std::size_t total, std::size_t workerCount )
{
  std::cout << "============================================================\n";
  std::cout << "Blue Test Runner\n";
  std::cout << "============================================================\n";
  std::cout << "Registered test executables: " << total << "\n";
  std::cout << "Execution mode             : " << GetExecutionModeName( workerCount ) << "\n";
  std::cout << "Workers                    : " << workerCount << "\n";
}

#if defined( _WIN32 )

class UniqueHandle
{
  public:
  UniqueHandle( ) = default;

  explicit UniqueHandle( HANDLE handle )
      : Handle( handle )
  {}

  ~UniqueHandle( ) { Reset( ); }

  UniqueHandle( const UniqueHandle& ) = delete;
  UniqueHandle& operator=( const UniqueHandle& ) = delete;

  UniqueHandle( UniqueHandle&& other ) noexcept
      : Handle( other.Release( ) )
  {}

  UniqueHandle& operator=( UniqueHandle&& other ) noexcept
  {
    if ( this != &other )
    {
      Reset( other.Release( ) );
    }

    return *this;
  }

  HANDLE Get( ) const { return Handle; }

  HANDLE Release( )
  {
    HANDLE handle = Handle;
    Handle = nullptr;
    return handle;
  }

  void Reset( HANDLE handle = nullptr )
  {
    if ( Handle != nullptr && Handle != INVALID_HANDLE_VALUE )
    {
      CloseHandle( Handle );
    }

    Handle = handle;
  }

  explicit operator bool( ) const { return Handle != nullptr && Handle != INVALID_HANDLE_VALUE; }

  private:
  HANDLE Handle = nullptr;
};

std::string QuoteCommandArgument( const std::string& value )
{
  if ( value.empty( ) )
  {
    return "\"\"";
  }

  bool needsQuotes = false;
  for ( char character : value )
  {
    if ( character == ' ' || character == '\t' || character == '"' )
    {
      needsQuotes = true;
      break;
    }
  }

  if ( !needsQuotes )
  {
    return value;
  }

  std::string result;
  result.reserve( value.size( ) + 2 );
  result.push_back( '"' );

  std::size_t backslashCount = 0;

  for ( char character : value )
  {
    if ( character == '\\' )
    {
      ++backslashCount;
      continue;
    }

    if ( character == '"' )
    {
      result.append( backslashCount * 2 + 1, '\\' );
      result.push_back( '"' );
      backslashCount = 0;
      continue;
    }

    result.append( backslashCount, '\\' );
    backslashCount = 0;
    result.push_back( character );
  }

  result.append( backslashCount * 2, '\\' );
  result.push_back( '"' );

  return result;
}

ProcessResult RunProcess( const std::string& executablePath )
{
  ProcessResult result;

  SECURITY_ATTRIBUTES securityAttributes{ };
  securityAttributes.nLength = sizeof( securityAttributes );
  securityAttributes.bInheritHandle = TRUE;

  HANDLE readPipeRaw = nullptr;
  HANDLE writePipeRaw = nullptr;

  if ( !CreatePipe( &readPipeRaw, &writePipeRaw, &securityAttributes, 0 ) )
  {
    result.Output = "[BlueRunTests] CreatePipe failed. error=" + std::to_string( GetLastError( ) ) + "\n";
    return result;
  }

  UniqueHandle readPipe( readPipeRaw );
  UniqueHandle writePipe( writePipeRaw );

  if ( !SetHandleInformation( readPipe.Get( ), HANDLE_FLAG_INHERIT, 0 ) )
  {
    result.Output = "[BlueRunTests] SetHandleInformation failed. error=" + std::to_string( GetLastError( ) ) + "\n";
    return result;
  }

  STARTUPINFOA startupInfo{ };
  startupInfo.cb = sizeof( startupInfo );
  startupInfo.dwFlags = STARTF_USESTDHANDLES;
  startupInfo.hStdInput = GetStdHandle( STD_INPUT_HANDLE );
  startupInfo.hStdOutput = writePipe.Get( );
  startupInfo.hStdError = writePipe.Get( );

  PROCESS_INFORMATION processInfo{ };

  std::string commandLine = QuoteCommandArgument( executablePath );

  const BOOL created = CreateProcessA( executablePath.c_str( ),
                                       commandLine.data( ),
                                       nullptr,
                                       nullptr,
                                       TRUE,
                                       0,
                                       nullptr,
                                       nullptr,
                                       &startupInfo,
                                       &processInfo );

  if ( !created )
  {
    result.Output = "[BlueRunTests] CreateProcess failed for: " + executablePath +
                    " error=" + std::to_string( GetLastError( ) ) + "\n";
    return result;
  }

  UniqueHandle processHandle( processInfo.hProcess );
  UniqueHandle threadHandle( processInfo.hThread );

  writePipe.Reset( );

  char buffer[ 4096 ];
  DWORD bytesRead = 0;

  while ( ReadFile( readPipe.Get( ), buffer, static_cast< DWORD >( sizeof( buffer ) ), &bytesRead, nullptr ) &&
          bytesRead > 0 )
  {
    result.Output.append( buffer, buffer + bytesRead );
  }

  WaitForSingleObject( processHandle.Get( ), INFINITE );

  DWORD exitCode = 1;
  if ( !GetExitCodeProcess( processHandle.Get( ), &exitCode ) )
  {
    result.Output += "[BlueRunTests] GetExitCodeProcess failed. error=" + std::to_string( GetLastError( ) ) + "\n";
    exitCode = 1;
  }

  result.ExitCode = static_cast< int >( exitCode );
  return result;
}

#else

class FileDescriptor
{
  public:
  FileDescriptor( ) = default;

  explicit FileDescriptor( int descriptor )
      : Descriptor( descriptor )
  {}

  ~FileDescriptor( ) { Reset( ); }

  FileDescriptor( const FileDescriptor& ) = delete;
  FileDescriptor& operator=( const FileDescriptor& ) = delete;

  FileDescriptor( FileDescriptor&& other ) noexcept
      : Descriptor( other.Release( ) )
  {}

  FileDescriptor& operator=( FileDescriptor&& other ) noexcept
  {
    if ( this != &other )
    {
      Reset( other.Release( ) );
    }

    return *this;
  }

  int Get( ) const { return Descriptor; }

  int Release( )
  {
    const int descriptor = Descriptor;
    Descriptor = -1;
    return descriptor;
  }

  void Reset( int descriptor = -1 )
  {
    if ( Descriptor >= 0 )
    {
      close( Descriptor );
    }

    Descriptor = descriptor;
  }

  explicit operator bool( ) const { return Descriptor >= 0; }

  private:
  int Descriptor = -1;
};

ProcessResult RunProcess( const std::string& executablePath )
{
  ProcessResult result;

  int pipeHandles[ 2 ] = { -1, -1 };
  if ( pipe( pipeHandles ) != 0 )
  {
    result.Output = "[BlueRunTests] pipe failed for: " + executablePath + "\n";
    return result;
  }

  FileDescriptor readPipe( pipeHandles[ 0 ] );
  FileDescriptor writePipe( pipeHandles[ 1 ] );

  const pid_t pid = fork( );

  if ( pid < 0 )
  {
    result.Output = "[BlueRunTests] fork failed for: " + executablePath + "\n";
    return result;
  }

  if ( pid == 0 )
  {
    readPipe.Reset( );

    dup2( writePipe.Get( ), STDOUT_FILENO );
    dup2( writePipe.Get( ), STDERR_FILENO );

    if ( writePipe.Get( ) > STDERR_FILENO )
    {
      writePipe.Reset( );
    }

    execl( executablePath.c_str( ), executablePath.c_str( ), static_cast< char* >( nullptr ) );
    _exit( 127 );
  }

  writePipe.Reset( );

  char buffer[ 4096 ];

  for ( ;; )
  {
    const ssize_t bytesRead = read( readPipe.Get( ), buffer, sizeof( buffer ) );

    if ( bytesRead > 0 )
    {
      result.Output.append( buffer, buffer + bytesRead );
      continue;
    }

    if ( bytesRead == 0 )
    {
      break;
    }

    if ( errno == EINTR )
    {
      continue;
    }

    result.Output += "[BlueRunTests] read failed for: " + executablePath + "\n";
    break;
  }

  readPipe.Reset( );

  int status = 0;
  while ( waitpid( pid, &status, 0 ) < 0 )
  {
    if ( errno == EINTR )
    {
      continue;
    }

    result.Output += "[BlueRunTests] waitpid failed for: " + executablePath + "\n";
    result.ExitCode = 1;
    return result;
  }

  if ( WIFEXITED( status ) )
  {
    result.ExitCode = WEXITSTATUS( status );
    return result;
  }

  if ( WIFSIGNALED( status ) )
  {
    result.ExitCode = 128 + WTERMSIG( status );
    return result;
  }

  result.ExitCode = 1;
  return result;
}

#endif

TestResult RunTestExecutable( const std::string& executablePath )
{
  TestResult result;
  result.ExecutablePath = executablePath;

  if ( !std::filesystem::exists( executablePath ) )
  {
    result.Missing = true;
    result.Started = false;
    result.ExitCode = 1;
    result.Output = "[BlueRunTests] Result: MISSING\n";
    return result;
  }

  result.Started = true;

  const auto startTime = std::chrono::steady_clock::now( );
  const ProcessResult processResult = RunProcess( executablePath );
  const auto endTime = std::chrono::steady_clock::now( );

  const std::chrono::duration< double, std::milli > elapsed = endTime - startTime;

  result.ExitCode = processResult.ExitCode;
  result.Output = processResult.Output;
  result.DurationMilliseconds = elapsed.count( );

  return result;
}

std::vector< TestResult > RunSequential( const std::vector< std::string >& testExecutables )
{
  std::vector< TestResult > results;
  results.reserve( testExecutables.size( ) );

  for ( const std::string& executablePath : testExecutables )
  {
    results.push_back( RunTestExecutable( executablePath ) );
  }

  return results;
}

std::vector< TestResult > RunParallel( const std::vector< std::string >& testExecutables, std::size_t workerCount )
{
  std::vector< TestResult > results( testExecutables.size( ) );
  std::atomic< std::size_t > nextIndex{ 0 };

  std::vector< std::thread > workers;
  workers.reserve( workerCount );

  for ( std::size_t workerIndex = 0; workerIndex < workerCount; ++workerIndex )
  {
    workers.emplace_back(
      [ &results, &testExecutables, &nextIndex ]( )
      {
        for ( ;; )
        {
          const std::size_t testIndex = nextIndex.fetch_add( 1 );

          if ( testIndex >= testExecutables.size( ) )
          {
            break;
          }

          results[ testIndex ] = RunTestExecutable( testExecutables[ testIndex ] );
        }
      } );
  }

  for ( std::thread& worker : workers )
  {
    worker.join( );
  }

  return results;
}

RunSummary RunTests( const std::vector< std::string >& testExecutables, std::size_t workerCount )
{
  RunSummary summary;
  summary.WorkerCount = workerCount;

  const auto startTime = std::chrono::steady_clock::now( );

  if ( workerCount <= 1 || testExecutables.size( ) <= 1 )
  {
    summary.Results = RunSequential( testExecutables );
  }
  else
  {
    summary.Results = RunParallel( testExecutables, workerCount );
  }

  const auto endTime = std::chrono::steady_clock::now( );
  const std::chrono::duration< double, std::milli > elapsed = endTime - startTime;
  summary.WallDurationMilliseconds = elapsed.count( );

  return summary;
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

void PrintResult( const TestResult& result, std::size_t index, std::size_t total )
{
  PrintLine( );

  std::cout << "[BlueRunTests] Test " << index << '/' << total << "\n";
  std::cout << "[BlueRunTests] Executable: " << result.ExecutablePath << "\n";

  PrintCapturedOutput( result.Output );

  if ( result.ExitCode == 0 )
  {
    std::cout << "[BlueRunTests] Result: PASS"
              << " duration_ms=" << result.DurationMilliseconds << "\n";
  }
  else if ( result.Missing )
  {
    std::cerr << "[BlueRunTests] Result: MISSING"
              << " duration_ms=" << result.DurationMilliseconds << "\n";
  }
  else
  {
    std::cerr << "[BlueRunTests] Result: FAIL"
              << " exit=" << result.ExitCode << " duration_ms=" << result.DurationMilliseconds << "\n";
  }

  if ( result.DurationMilliseconds >= SlowExecutableWarningMilliseconds )
  {
    std::cout << "[BlueRunTests] Warning: slow test executable"
              << " duration_ms=" << result.DurationMilliseconds << " executable=" << result.ExecutablePath << "\n";
  }
}

void PrintResults( const std::vector< TestResult >& results )
{
  for ( std::size_t index = 0; index < results.size( ); ++index )
  {
    PrintResult( results[ index ], index + 1, results.size( ) );
  }
}

void PrintSlowestExecutables( const std::vector< TestResult >& results )
{
  if ( results.empty( ) )
  {
    return;
  }

  std::vector< std::size_t > indices;
  indices.reserve( results.size( ) );

  for ( std::size_t index = 0; index < results.size( ); ++index )
  {
    indices.push_back( index );
  }

  std::sort( indices.begin( ),
             indices.end( ),
             [ &results ]( std::size_t left, std::size_t right )
             {
               return results[ left ].DurationMilliseconds > results[ right ].DurationMilliseconds;
             } );

  std::cout << "  Slowest    :\n";

  const std::size_t count = std::min( SlowExecutableReportCount, indices.size( ) );
  for ( std::size_t rank = 0; rank < count; ++rank )
  {
    const TestResult& result = results[ indices[ rank ] ];
    std::cout << "    " << rank + 1 << ". " << GetFileName( result.ExecutablePath )
              << " duration_ms=" << result.DurationMilliseconds << "\n";
  }
}

void PrintSummary( const RunSummary& summary )
{
  const std::uint32_t failedCount = CountFailedTests( summary.Results );
  const std::uint32_t passedCount = static_cast< std::uint32_t >( summary.Results.size( ) ) - failedCount;

  PrintLine( );
  std::cout << "Blue Test Runner Summary\n";
  std::cout << "  Total      : " << summary.Results.size( ) << "\n";
  std::cout << "  Passed     : " << passedCount << "\n";
  std::cout << "  Failed     : " << failedCount << "\n";
  std::cout << "  Mode       : " << GetExecutionModeName( summary.WorkerCount ) << "\n";
  std::cout << "  Workers    : " << summary.WorkerCount << "\n";
  std::cout << "  Wall ms    : " << summary.WallDurationMilliseconds << "\n";
  std::cout << "  Sum ms     : " << SumDurations( summary.Results ) << "\n";
  PrintSlowestExecutables( summary.Results );
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

void PrintTestList( const std::vector< std::string >& testExecutables )
{
  PrintLine( );
  std::cout << "Registered test executables:\n";

  for ( std::size_t index = 0; index < testExecutables.size( ); ++index )
  {
    std::cout << "  " << index + 1 << ". " << testExecutables[ index ] << "\n";
  }

  PrintLine( );
}
} // namespace

int main( int argc, char** argv )
{
  ConfigureChildTestEnvironment( );

  const ParsedArguments parsed = ParseArguments( argc, argv );

  if ( !parsed.Success )
  {
    std::cerr << "[BlueRunTests] " << parsed.ErrorMessage << "\n";
    std::cerr << "Use --help for usage.\n";
    return parsed.ExitCode;
  }

  if ( parsed.Options.Help )
  {
    PrintUsage( );
    return 0;
  }

  const std::size_t workerCount = ResolveWorkerCount( parsed.Options, parsed.TestExecutables.size( ) );

  PrintHeader( parsed.TestExecutables.size( ), workerCount );

  if ( parsed.Options.ListOnly )
  {
    PrintTestList( parsed.TestExecutables );
    return 0;
  }

  const RunSummary summary = RunTests( parsed.TestExecutables, workerCount );

  PrintResults( summary.Results );
  PrintSummary( summary );

  return CountFailedTests( summary.Results ) == 0 ? 0 : 1;
}

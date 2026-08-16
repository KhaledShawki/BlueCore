#include <Blue/System/Platform/WindowsLean.h>

#include <algorithm>
#include <chrono>
#include <climits>
#include <cstdint>
#include <filesystem>
#include <limits>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

#include "BoundedOutputBuffer.h"
#include "ProcessRunner.h"

namespace BlueTestRunner
{
namespace
{
constexpr std::chrono::milliseconds WaitPollInterval{ 20 };
constexpr std::chrono::milliseconds CleanupDrainLimit{ 1000 };
constexpr std::chrono::milliseconds CleanupTerminationLimit{ 5000 };
constexpr DWORD TimeoutExitCode = 124;
constexpr DWORD MaximumDrainBytesPerIteration = 256U * 1024U;

class UniqueHandle
{
public:
  UniqueHandle( ) = default;
  explicit UniqueHandle( HANDLE handle )
      : m_handle( handle )
  {}

  ~UniqueHandle( ) { Reset( ); }

  UniqueHandle( const UniqueHandle& ) = delete;
  UniqueHandle& operator=( const UniqueHandle& ) = delete;

  UniqueHandle( UniqueHandle&& other ) noexcept
      : m_handle( other.Release( ) )
  {}

  UniqueHandle& operator=( UniqueHandle&& other ) noexcept
  {
    if ( this != &other )
    {
      Reset( other.Release( ) );
    }
    return *this;
  }

  [[nodiscard]] HANDLE Get( ) const { return m_handle; }
  [[nodiscard]] explicit operator bool( ) const { return m_handle != nullptr && m_handle != INVALID_HANDLE_VALUE; }

  HANDLE Release( )
  {
    HANDLE handle = m_handle;
    m_handle = nullptr;
    return handle;
  }

  void Reset( HANDLE handle = nullptr )
  {
    if ( m_handle != nullptr && m_handle != INVALID_HANDLE_VALUE )
    {
      CloseHandle( m_handle );
    }
    m_handle = handle;
  }

private:
  HANDLE m_handle = nullptr;
};

std::string WindowsErrorMessage( const DWORD error )
{
  return std::system_category( ).message( static_cast< int >( error ) );
}

bool TryUtf8ToWide( const std::string_view value, std::wstring& result, std::string& errorMessage )
{
  result.clear( );

  if ( value.empty( ) )
  {
    return true;
  }

  if ( value.size( ) > static_cast< std::size_t >( INT_MAX ) )
  {
    errorMessage = "UTF-8 command argument exceeds the Windows conversion limit.";
    return false;
  }

  const int required = MultiByteToWideChar( CP_UTF8,
                                            MB_ERR_INVALID_CHARS,
                                            value.data( ),
                                            static_cast< int >( value.size( ) ),
                                            nullptr,
                                            0 );
  if ( required <= 0 )
  {
    errorMessage = "UTF-8 conversion failed: " + WindowsErrorMessage( GetLastError( ) );
    return false;
  }

  result.resize( static_cast< std::size_t >( required ) );
  if ( MultiByteToWideChar( CP_UTF8,
                            MB_ERR_INVALID_CHARS,
                            value.data( ),
                            static_cast< int >( value.size( ) ),
                            result.data( ),
                            required ) != required )
  {
    errorMessage = "UTF-8 conversion failed: " + WindowsErrorMessage( GetLastError( ) );
    result.clear( );
    return false;
  }

  return true;
}

std::wstring QuoteCommandArgument( const std::wstring_view value )
{
  if ( value.empty( ) )
  {
    return L"\"\"";
  }

  const bool needsQuotes = value.find_first_of( L" \t\"" ) != std::wstring_view::npos;
  if ( !needsQuotes )
  {
    return std::wstring( value );
  }

  std::wstring result;
  result.reserve( value.size( ) + 2 );
  result.push_back( L'\"' );

  std::size_t backslashCount = 0;
  for ( const wchar_t character : value )
  {
    if ( character == L'\\' )
    {
      ++backslashCount;
      continue;
    }

    if ( character == L'\"' )
    {
      result.append( backslashCount * 2 + 1, L'\\' );
      result.push_back( L'\"' );
      backslashCount = 0;
      continue;
    }

    result.append( backslashCount, L'\\' );
    backslashCount = 0;
    result.push_back( character );
  }

  result.append( backslashCount * 2, L'\\' );
  result.push_back( L'\"' );
  return result;
}

bool BuildCommandLine( const std::filesystem::path& executablePath,
                       const std::vector< std::string >& arguments,
                       std::wstring& commandLine,
                       std::string& errorMessage )
{
  commandLine = QuoteCommandArgument( executablePath.wstring( ) );

  for ( const std::string& argument : arguments )
  {
    std::wstring wideArgument;
    if ( !TryUtf8ToWide( argument, wideArgument, errorMessage ) )
    {
      return false;
    }

    commandLine.push_back( L' ' );
    commandLine.append( QuoteCommandArgument( wideArgument ) );
  }

  return true;
}

bool ConfigureKillOnClose( const HANDLE job, std::string& errorMessage )
{
  JOBOBJECT_EXTENDED_LIMIT_INFORMATION limits{ };
  limits.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
  if ( !SetInformationJobObject( job, JobObjectExtendedLimitInformation, &limits, sizeof( limits ) ) )
  {
    errorMessage = "SetInformationJobObject failed: " + WindowsErrorMessage( GetLastError( ) );
    return false;
  }
  return true;
}

bool DrainAvailableOutput( const HANDLE readPipe,
                           BoundedOutputBuffer& output,
                           const DWORD maximumBytes,
                           bool& pipeClosed,
                           std::string& errorMessage )
{
  DWORD drained = 0;
  char buffer[ 4096 ];

  while ( drained < maximumBytes )
  {
    DWORD available = 0;
    if ( !PeekNamedPipe( readPipe, nullptr, 0, nullptr, &available, nullptr ) )
    {
      const DWORD error = GetLastError( );
      if ( error == ERROR_BROKEN_PIPE )
      {
        pipeClosed = true;
        return true;
      }
      errorMessage = "PeekNamedPipe failed: " + WindowsErrorMessage( error );
      return false;
    }

    if ( available == 0 )
    {
      return true;
    }

    const DWORD allowed = maximumBytes - drained;
    const DWORD toRead = std::min< DWORD >( static_cast< DWORD >( sizeof( buffer ) ), std::min( available, allowed ) );
    DWORD bytesRead = 0;
    if ( !ReadFile( readPipe, buffer, toRead, &bytesRead, nullptr ) )
    {
      const DWORD error = GetLastError( );
      if ( error == ERROR_BROKEN_PIPE )
      {
        pipeClosed = true;
        return true;
      }
      errorMessage = "ReadFile(test output) failed: " + WindowsErrorMessage( error );
      return false;
    }

    if ( bytesRead == 0 )
    {
      return true;
    }

    output.Append( buffer, static_cast< std::size_t >( bytesRead ) );
    drained += bytesRead;
  }

  return true;
}

bool DrainAfterTermination( const HANDLE readPipe,
                            BoundedOutputBuffer& output,
                            bool& pipeClosed,
                            std::string& errorMessage )
{
  const auto deadline = std::chrono::steady_clock::now( ) + CleanupDrainLimit;
  while ( !pipeClosed && std::chrono::steady_clock::now( ) < deadline )
  {
    if ( !DrainAvailableOutput( readPipe, output, MaximumDrainBytesPerIteration, pipeClosed, errorMessage ) )
    {
      return false;
    }
    if ( pipeClosed )
    {
      return true;
    }
    Sleep( static_cast< DWORD >( WaitPollInterval.count( ) ) );
  }

  if ( !pipeClosed )
  {
    errorMessage = "test output pipe did not close after process-tree termination";
    return false;
  }
  return true;
}

DWORD WaitSliceMilliseconds( const std::chrono::steady_clock::time_point deadline )
{
  const auto now = std::chrono::steady_clock::now( );
  if ( now >= deadline )
  {
    return 0;
  }

  const auto remaining = std::chrono::duration_cast< std::chrono::milliseconds >( deadline - now );
  return static_cast< DWORD >(
    std::max< std::int64_t >( 1, std::min< std::int64_t >( WaitPollInterval.count( ), remaining.count( ) ) ) );
}

void AppendError( std::string& destination, const std::string& message )
{
  if ( message.empty( ) )
  {
    return;
  }
  if ( !destination.empty( ) )
  {
    destination.append( "; " );
  }
  destination.append( message );
}

bool WaitForForcedTermination( const HANDLE process, std::string& errorMessage )
{
  const DWORD waitResult = WaitForSingleObject( process, static_cast< DWORD >( CleanupTerminationLimit.count( ) ) );
  if ( waitResult == WAIT_OBJECT_0 )
  {
    return true;
  }
  if ( waitResult == WAIT_TIMEOUT )
  {
    errorMessage = "process did not terminate within the forced-cleanup deadline";
    return false;
  }

  errorMessage = "WaitForSingleObject during forced cleanup failed: " + WindowsErrorMessage( GetLastError( ) );
  return false;
}

bool TerminateProcessAndWait( const HANDLE process, const DWORD exitCode, std::string& errorMessage )
{
  if ( !TerminateProcess( process, exitCode ) )
  {
    const DWORD error = GetLastError( );
    if ( WaitForSingleObject( process, 0 ) != WAIT_OBJECT_0 )
    {
      errorMessage = "TerminateProcess failed: " + WindowsErrorMessage( error );
      return false;
    }
  }

  return WaitForForcedTermination( process, errorMessage );
}

bool TerminateAndWait( const HANDLE job, const HANDLE process, std::string& errorMessage )
{
  if ( !TerminateJobObject( job, TimeoutExitCode ) )
  {
    const DWORD error = GetLastError( );
    if ( WaitForSingleObject( process, 0 ) != WAIT_OBJECT_0 )
    {
      errorMessage = "TerminateJobObject failed: " + WindowsErrorMessage( error );
      return false;
    }
  }

  return WaitForForcedTermination( process, errorMessage );
}
} // namespace

void ConfigureChildTestEnvironment( )
{
  SetEnvironmentVariableW( L"GTEST_COLOR", L"yes" );
}

ProcessResult RunProcess( const std::filesystem::path& executablePath,
                          const std::vector< std::string >& arguments,
                          const ProcessOptions& options )
{
  ProcessResult result;
  BoundedOutputBuffer output( options.maximumOutputBytes );

  SECURITY_ATTRIBUTES securityAttributes{ };
  securityAttributes.nLength = sizeof( securityAttributes );
  securityAttributes.bInheritHandle = TRUE;

  HANDLE readPipeRaw = nullptr;
  HANDLE writePipeRaw = nullptr;
  if ( !CreatePipe( &readPipeRaw, &writePipeRaw, &securityAttributes, 0 ) )
  {
    result.status = ProcessStatus::RunnerError;
    result.errorMessage = "CreatePipe failed: " + WindowsErrorMessage( GetLastError( ) );
    return result;
  }

  UniqueHandle readPipe( readPipeRaw );
  UniqueHandle writePipe( writePipeRaw );
  if ( !SetHandleInformation( readPipe.Get( ), HANDLE_FLAG_INHERIT, 0 ) )
  {
    result.status = ProcessStatus::RunnerError;
    result.errorMessage = "SetHandleInformation failed: " + WindowsErrorMessage( GetLastError( ) );
    return result;
  }

  UniqueHandle job( CreateJobObjectW( nullptr, nullptr ) );
  if ( !job )
  {
    result.status = ProcessStatus::RunnerError;
    result.errorMessage = "CreateJobObject failed: " + WindowsErrorMessage( GetLastError( ) );
    return result;
  }
  if ( !ConfigureKillOnClose( job.Get( ), result.errorMessage ) )
  {
    result.status = ProcessStatus::RunnerError;
    return result;
  }

  STARTUPINFOW startupInfo{ };
  startupInfo.cb = sizeof( startupInfo );
  startupInfo.dwFlags = STARTF_USESTDHANDLES;
  startupInfo.hStdInput = GetStdHandle( STD_INPUT_HANDLE );
  startupInfo.hStdOutput = writePipe.Get( );
  startupInfo.hStdError = writePipe.Get( );

  PROCESS_INFORMATION processInfo{ };
  std::wstring commandLine;
  if ( !BuildCommandLine( executablePath, arguments, commandLine, result.errorMessage ) )
  {
    result.status = ProcessStatus::LaunchFailed;
    return result;
  }

  const std::wstring executable = executablePath.wstring( );
  if ( !CreateProcessW( executable.c_str( ),
                        commandLine.data( ),
                        nullptr,
                        nullptr,
                        TRUE,
                        CREATE_SUSPENDED,
                        nullptr,
                        nullptr,
                        &startupInfo,
                        &processInfo ) )
  {
    result.status = ProcessStatus::LaunchFailed;
    result.errorMessage = "CreateProcessW failed: " + WindowsErrorMessage( GetLastError( ) );
    return result;
  }

  UniqueHandle process( processInfo.hProcess );
  UniqueHandle primaryThread( processInfo.hThread );

  if ( !AssignProcessToJobObject( job.Get( ), process.Get( ) ) )
  {
    const DWORD error = GetLastError( );
    std::string cleanupError;
    TerminateProcessAndWait( process.Get( ), 1, cleanupError );
    result.status = ProcessStatus::RunnerError;
    result.errorMessage = "AssignProcessToJobObject failed: " + WindowsErrorMessage( error );
    AppendError( result.errorMessage, cleanupError );
    return result;
  }

  writePipe.Reset( );

  if ( ResumeThread( primaryThread.Get( ) ) == static_cast< DWORD >( -1 ) )
  {
    const DWORD error = GetLastError( );
    std::string cleanupError;
    TerminateAndWait( job.Get( ), process.Get( ), cleanupError );
    result.status = ProcessStatus::RunnerError;
    result.errorMessage = "ResumeThread failed: " + WindowsErrorMessage( error );
    AppendError( result.errorMessage, cleanupError );
    return result;
  }
  primaryThread.Reset( );

  bool pipeClosed = false;
  const auto deadline = std::chrono::steady_clock::now( ) + options.timeout;

  for ( ;; )
  {
    if ( !DrainAvailableOutput( readPipe.Get( ),
                                output,
                                MaximumDrainBytesPerIteration,
                                pipeClosed,
                                result.errorMessage ) )
    {
      std::string cleanupError;
      TerminateAndWait( job.Get( ), process.Get( ), cleanupError );
      AppendError( result.errorMessage, cleanupError );
      result.status = ProcessStatus::RunnerError;
      break;
    }

    const DWORD waitResult = WaitForSingleObject( process.Get( ), WaitSliceMilliseconds( deadline ) );
    if ( waitResult == WAIT_OBJECT_0 )
    {
      DWORD exitCode = 1;
      if ( !GetExitCodeProcess( process.Get( ), &exitCode ) )
      {
        result.status = ProcessStatus::RunnerError;
        result.errorMessage = "GetExitCodeProcess failed: " + WindowsErrorMessage( GetLastError( ) );
      }
      else
      {
        result.status = ProcessStatus::Exited;
        result.exitCode = static_cast< int >( exitCode );
      }

      // Closing a kill-on-close job after the primary process exits terminates any
      // descendants that would otherwise outlive the test or hold the output pipe open.
      job.Reset( );
      break;
    }

    if ( waitResult == WAIT_FAILED )
    {
      const DWORD error = GetLastError( );
      std::string cleanupError;
      TerminateAndWait( job.Get( ), process.Get( ), cleanupError );
      result.status = ProcessStatus::RunnerError;
      result.errorMessage = "WaitForSingleObject failed: " + WindowsErrorMessage( error );
      AppendError( result.errorMessage, cleanupError );
      job.Reset( );
      break;
    }

    if ( std::chrono::steady_clock::now( ) >= deadline )
    {
      std::string cleanupError;
      if ( !TerminateAndWait( job.Get( ), process.Get( ), cleanupError ) )
      {
        result.status = ProcessStatus::RunnerError;
        result.errorMessage = "test timed out; " + cleanupError;
        result.exitCode = 1;
      }
      else
      {
        result.status = ProcessStatus::TimedOut;
        result.exitCode = static_cast< int >( TimeoutExitCode );
      }
      job.Reset( );
      break;
    }
  }

  std::string cleanupError;
  if ( !DrainAfterTermination( readPipe.Get( ), output, pipeClosed, cleanupError ) )
  {
    if ( !result.errorMessage.empty( ) )
    {
      result.errorMessage.append( "; " );
    }
    result.errorMessage.append( cleanupError );
    result.status = ProcessStatus::RunnerError;
    result.exitCode = 1;
  }
  result.output = output.BuildOutput( );
  result.producedOutputBytes = output.ProducedBytes( );
  result.outputTruncated = output.Truncated( );
  return result;
}
} // namespace BlueTestRunner

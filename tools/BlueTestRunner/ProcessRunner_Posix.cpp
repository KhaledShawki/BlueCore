#include <algorithm>
#include <cerrno>
#include <chrono>
#include <csignal>
#include <cstdint>
#include <cstdlib>
#include <string>
#include <system_error>
#include <vector>

#include "BoundedOutputBuffer.h"
#include "ProcessRunner.h"

#include <fcntl.h>
#include <poll.h>
#include <sys/wait.h>
#include <unistd.h>

namespace BlueTestRunner
{
namespace
{
constexpr std::chrono::milliseconds PollInterval{ 20 };
constexpr std::chrono::milliseconds CleanupDrainLimit{ 1000 };
constexpr std::chrono::milliseconds CleanupTerminationLimit{ 5000 };
constexpr std::size_t MaximumDrainBytesPerIteration = 256U * 1024U;
constexpr int TimeoutExitCode = 124;

class FileDescriptor
{
public:
  FileDescriptor( ) = default;
  explicit FileDescriptor( const int descriptor )
      : m_descriptor( descriptor )
  {}

  ~FileDescriptor( ) { Reset( ); }

  FileDescriptor( const FileDescriptor& ) = delete;
  FileDescriptor& operator=( const FileDescriptor& ) = delete;

  FileDescriptor( FileDescriptor&& other ) noexcept
      : m_descriptor( other.Release( ) )
  {}

  FileDescriptor& operator=( FileDescriptor&& other ) noexcept
  {
    if ( this != &other )
    {
      Reset( other.Release( ) );
    }
    return *this;
  }

  [[nodiscard]] int Get( ) const { return m_descriptor; }
  [[nodiscard]] explicit operator bool( ) const { return m_descriptor >= 0; }

  int Release( )
  {
    const int descriptor = m_descriptor;
    m_descriptor = -1;
    return descriptor;
  }

  void Reset( const int descriptor = -1 )
  {
    if ( m_descriptor >= 0 )
    {
      close( m_descriptor );
    }
    m_descriptor = descriptor;
  }

private:
  int m_descriptor = -1;
};

std::string ErrorMessage( const int error )
{
  return std::error_code( error, std::generic_category( ) ).message( );
}

bool CreatePipe( FileDescriptor& readHandle, FileDescriptor& writeHandle, std::string& errorMessage )
{
  int handles[ 2 ] = { -1, -1 };
  if ( pipe( handles ) != 0 )
  {
    errorMessage = "pipe failed: " + ErrorMessage( errno );
    return false;
  }

  readHandle.Reset( handles[ 0 ] );
  writeHandle.Reset( handles[ 1 ] );
  return true;
}

bool SetNonBlocking( const int descriptor, std::string& errorMessage )
{
  const int flags = fcntl( descriptor, F_GETFL, 0 );
  if ( flags < 0 || fcntl( descriptor, F_SETFL, flags | O_NONBLOCK ) < 0 )
  {
    errorMessage = "fcntl(O_NONBLOCK) failed: " + ErrorMessage( errno );
    return false;
  }
  return true;
}

bool SetCloseOnExec( const int descriptor, std::string& errorMessage )
{
  const int flags = fcntl( descriptor, F_GETFD, 0 );
  if ( flags < 0 || fcntl( descriptor, F_SETFD, flags | FD_CLOEXEC ) < 0 )
  {
    errorMessage = "fcntl(FD_CLOEXEC) failed: " + ErrorMessage( errno );
    return false;
  }
  return true;
}

void ReportChildLaunchFailure( const int descriptor, const int error )
{
  const int savedError = error;
  const auto* data = reinterpret_cast< const char* >( &savedError );
  std::size_t remaining = sizeof( savedError );

  while ( remaining > 0 )
  {
    const ssize_t written = write( descriptor, data + sizeof( savedError ) - remaining, remaining );
    if ( written > 0 )
    {
      remaining -= static_cast< std::size_t >( written );
      continue;
    }
    if ( written < 0 && errno == EINTR )
    {
      continue;
    }
    break;
  }
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

bool KillProcessTree( const pid_t pid, std::string& errorMessage )
{
  bool success = true;

  if ( pid > 0 && kill( -pid, SIGKILL ) != 0 && errno != ESRCH )
  {
    AppendError( errorMessage, "kill(process group) failed: " + ErrorMessage( errno ) );
    success = false;
  }

  // The direct signal also covers the narrow startup window before the child has
  // established its dedicated process group.
  if ( pid > 0 && kill( pid, SIGKILL ) != 0 && errno != ESRCH )
  {
    AppendError( errorMessage, "kill(process) failed: " + ErrorMessage( errno ) );
    success = false;
  }

  return success;
}

bool ReapProcessUntil( const pid_t pid,
                       int& status,
                       const std::chrono::steady_clock::time_point deadline,
                       std::string& errorMessage )
{
  for ( ;; )
  {
    const pid_t waited = waitpid( pid, &status, WNOHANG );
    if ( waited == pid )
    {
      return true;
    }
    if ( waited < 0 )
    {
      if ( errno == EINTR )
      {
        continue;
      }

      errorMessage = "waitpid failed: " + ErrorMessage( errno );
      return false;
    }

    const auto now = std::chrono::steady_clock::now( );
    if ( now >= deadline )
    {
      errorMessage = "process did not terminate within the forced-cleanup deadline";
      return false;
    }

    const auto remaining = std::chrono::duration_cast< std::chrono::milliseconds >( deadline - now );
    const int sleepMilliseconds = static_cast< int >(
      std::max< std::int64_t >( 1, std::min< std::int64_t >( PollInterval.count( ), remaining.count( ) ) ) );
    poll( nullptr, 0, sleepMilliseconds );
  }
}

bool TerminateAndReap( const pid_t pid, int& status, std::string& errorMessage )
{
  std::string killError;
  const bool killed = KillProcessTree( pid, killError );

  std::string reapError;
  const bool reaped =
    ReapProcessUntil( pid, status, std::chrono::steady_clock::now( ) + CleanupTerminationLimit, reapError );

  AppendError( errorMessage, killError );
  AppendError( errorMessage, reapError );
  return killed && reaped;
}

bool ObserveProcessExit( const pid_t pid, bool& exited, std::string& errorMessage )
{
  siginfo_t info{ };
  for ( ;; )
  {
    if ( waitid( P_PID, static_cast< id_t >( pid ), &info, WEXITED | WNOHANG | WNOWAIT ) == 0 )
    {
      exited = info.si_pid == pid;
      return true;
    }
    if ( errno == EINTR )
    {
      continue;
    }

    errorMessage = "waitid failed: " + ErrorMessage( errno );
    return false;
  }
}

bool DrainOutput( const int descriptor,
                  BoundedOutputBuffer& output,
                  const std::size_t maximumBytes,
                  bool& closed,
                  std::string& errorMessage )
{
  std::size_t drained = 0;
  char buffer[ 4096 ];

  while ( drained < maximumBytes )
  {
    const std::size_t allowed = std::min( sizeof( buffer ), maximumBytes - drained );
    const ssize_t bytesRead = read( descriptor, buffer, allowed );
    if ( bytesRead > 0 )
    {
      const std::size_t captured = static_cast< std::size_t >( bytesRead );
      output.Append( buffer, captured );
      drained += captured;
      continue;
    }
    if ( bytesRead == 0 )
    {
      closed = true;
      return true;
    }
    if ( errno == EINTR )
    {
      continue;
    }
    if ( errno == EAGAIN || errno == EWOULDBLOCK )
    {
      return true;
    }

    errorMessage = "read(test output) failed: " + ErrorMessage( errno );
    return false;
  }

  return true;
}

bool DrainExecError( const int descriptor,
                     bool& closed,
                     bool& launchFailed,
                     int& launchError,
                     std::size_t& bytesReceived,
                     std::string& errorMessage )
{
  auto* target = reinterpret_cast< char* >( &launchError );
  static_assert( sizeof( launchError ) <= 16 );

  while ( bytesReceived < sizeof( launchError ) )
  {
    const ssize_t bytesRead = read( descriptor, target + bytesReceived, sizeof( launchError ) - bytesReceived );
    if ( bytesRead > 0 )
    {
      bytesReceived += static_cast< std::size_t >( bytesRead );
      if ( bytesReceived == sizeof( launchError ) )
      {
        launchFailed = true;
        closed = true;
        return true;
      }
      continue;
    }
    if ( bytesRead == 0 )
    {
      closed = true;
      if ( bytesReceived != 0 )
      {
        errorMessage = "partial exec error payload received from child process";
        return false;
      }
      return true;
    }
    if ( errno == EINTR )
    {
      continue;
    }
    if ( errno == EAGAIN || errno == EWOULDBLOCK )
    {
      return true;
    }

    errorMessage = "read(exec error pipe) failed: " + ErrorMessage( errno );
    return false;
  }

  launchFailed = true;
  closed = true;
  return true;
}

bool DrainAfterTermination( const int outputDescriptor,
                            BoundedOutputBuffer& output,
                            bool& outputClosed,
                            std::string& errorMessage )
{
  const auto deadline = std::chrono::steady_clock::now( ) + CleanupDrainLimit;
  while ( !outputClosed && std::chrono::steady_clock::now( ) < deadline )
  {
    if ( !DrainOutput( outputDescriptor, output, MaximumDrainBytesPerIteration, outputClosed, errorMessage ) )
    {
      return false;
    }
    if ( outputClosed )
    {
      return true;
    }

    pollfd descriptor{ outputDescriptor, POLLIN | POLLHUP, 0 };
    const int pollResult = poll( &descriptor, 1, static_cast< int >( PollInterval.count( ) ) );
    if ( pollResult < 0 && errno != EINTR )
    {
      errorMessage = "poll during output cleanup failed: " + ErrorMessage( errno );
      return false;
    }
  }

  if ( !outputClosed )
  {
    errorMessage = "test output pipe did not close after process-tree termination";
    return false;
  }
  return true;
}

int ExitCodeFromStatus( const int status )
{
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
} // namespace

void ConfigureChildTestEnvironment( )
{
  setenv( "GTEST_COLOR", "yes", 1 );
}

ProcessResult RunProcess( const std::filesystem::path& executablePath,
                          const std::vector< std::string >& arguments,
                          const ProcessOptions& options )
{
  ProcessResult result;
  BoundedOutputBuffer output( options.maximumOutputBytes );

  FileDescriptor outputRead;
  FileDescriptor outputWrite;
  FileDescriptor execErrorRead;
  FileDescriptor execErrorWrite;

  if ( !CreatePipe( outputRead, outputWrite, result.errorMessage ) ||
       !CreatePipe( execErrorRead, execErrorWrite, result.errorMessage ) ||
       !SetNonBlocking( outputRead.Get( ), result.errorMessage ) ||
       !SetNonBlocking( execErrorRead.Get( ), result.errorMessage ) ||
       !SetCloseOnExec( execErrorWrite.Get( ), result.errorMessage ) )
  {
    result.status = ProcessStatus::RunnerError;
    return result;
  }

  const std::string executable = executablePath.string( );
  std::vector< char* > childArguments;
  childArguments.reserve( arguments.size( ) + 2 );
  childArguments.push_back( const_cast< char* >( executable.c_str( ) ) );
  for ( const std::string& argument : arguments )
  {
    childArguments.push_back( const_cast< char* >( argument.c_str( ) ) );
  }
  childArguments.push_back( nullptr );

  const pid_t pid = fork( );
  if ( pid < 0 )
  {
    result.status = ProcessStatus::LaunchFailed;
    result.errorMessage = "fork failed: " + ErrorMessage( errno );
    return result;
  }

  if ( pid == 0 )
  {
    outputRead.Reset( );
    execErrorRead.Reset( );

    if ( setpgid( 0, 0 ) != 0 )
    {
      ReportChildLaunchFailure( execErrorWrite.Get( ), errno );
      _exit( 127 );
    }

    if ( dup2( outputWrite.Get( ), STDOUT_FILENO ) < 0 || dup2( outputWrite.Get( ), STDERR_FILENO ) < 0 )
    {
      ReportChildLaunchFailure( execErrorWrite.Get( ), errno );
      _exit( 127 );
    }

    outputWrite.Reset( );
    execv( executable.c_str( ), childArguments.data( ) );
    ReportChildLaunchFailure( execErrorWrite.Get( ), errno );
    _exit( 127 );
  }

  outputWrite.Reset( );
  execErrorWrite.Reset( );

  if ( setpgid( pid, pid ) != 0 && errno != EACCES && errno != ESRCH )
  {
    const int groupError = errno;
    int ignoredStatus = 0;
    std::string cleanupError;
    TerminateAndReap( pid, ignoredStatus, cleanupError );
    result.status = ProcessStatus::RunnerError;
    result.errorMessage = "setpgid failed: " + ErrorMessage( groupError );
    AppendError( result.errorMessage, cleanupError );
    return result;
  }

  bool outputClosed = false;
  bool execErrorClosed = false;
  bool launchFailed = false;
  int launchError = 0;
  std::size_t launchErrorBytesReceived = 0;
  int processStatus = 0;
  const auto deadline = std::chrono::steady_clock::now( ) + options.timeout;

  for ( ;; )
  {
    if ( !outputClosed &&
         !DrainOutput( outputRead.Get( ), output, MaximumDrainBytesPerIteration, outputClosed, result.errorMessage ) )
    {
      std::string cleanupError;
      TerminateAndReap( pid, processStatus, cleanupError );
      AppendError( result.errorMessage, cleanupError );
      result.status = ProcessStatus::RunnerError;
      break;
    }
    if ( outputClosed )
    {
      outputRead.Reset( );
    }

    if ( !execErrorClosed && !DrainExecError( execErrorRead.Get( ),
                                              execErrorClosed,
                                              launchFailed,
                                              launchError,
                                              launchErrorBytesReceived,
                                              result.errorMessage ) )
    {
      std::string cleanupError;
      TerminateAndReap( pid, processStatus, cleanupError );
      AppendError( result.errorMessage, cleanupError );
      result.status = ProcessStatus::RunnerError;
      break;
    }
    if ( execErrorClosed )
    {
      execErrorRead.Reset( );
    }

    bool exited = false;
    if ( !ObserveProcessExit( pid, exited, result.errorMessage ) )
    {
      std::string cleanupError;
      TerminateAndReap( pid, processStatus, cleanupError );
      AppendError( result.errorMessage, cleanupError );
      result.status = ProcessStatus::RunnerError;
      break;
    }

    if ( exited )
    {
      // A child can exit after the non-blocking exec-error read above but before
      // waitid observes it. Once waitid reports exit, the child-side writer is
      // closed, so finalize the launch-status pipe before classifying the result.
      if ( !execErrorClosed )
      {
        if ( !DrainExecError( execErrorRead.Get( ),
                              execErrorClosed,
                              launchFailed,
                              launchError,
                              launchErrorBytesReceived,
                              result.errorMessage ) ||
             !execErrorClosed )
        {
          if ( result.errorMessage.empty( ) )
          {
            result.errorMessage = "exec error pipe remained open after child exit";
          }

          std::string cleanupError;
          TerminateAndReap( pid, processStatus, cleanupError );
          AppendError( result.errorMessage, cleanupError );
          result.status = ProcessStatus::RunnerError;
          break;
        }
        execErrorRead.Reset( );
      }

      // The primary process remains waitable because ObserveProcessExit uses WNOWAIT.
      // Terminate the process group before reaping it so descendants cannot outlive
      // the test or hold the captured-output pipe open indefinitely.
      std::string cleanupError;
      if ( !TerminateAndReap( pid, processStatus, cleanupError ) )
      {
        result.status = ProcessStatus::RunnerError;
        result.errorMessage = cleanupError;
      }
      else if ( launchFailed )
      {
        result.status = ProcessStatus::LaunchFailed;
        result.errorMessage = "exec failed: " + ErrorMessage( launchError );
      }
      else
      {
        result.status = ProcessStatus::Exited;
        result.exitCode = ExitCodeFromStatus( processStatus );
      }
      break;
    }

    if ( std::chrono::steady_clock::now( ) >= deadline )
    {
      std::string cleanupError;
      if ( !TerminateAndReap( pid, processStatus, cleanupError ) )
      {
        result.status = ProcessStatus::RunnerError;
        result.errorMessage = "test timed out; " + cleanupError;
      }
      else
      {
        result.status = ProcessStatus::TimedOut;
        result.exitCode = TimeoutExitCode;
      }
      break;
    }

    const auto remaining =
      std::chrono::duration_cast< std::chrono::milliseconds >( deadline - std::chrono::steady_clock::now( ) );
    const int timeout = static_cast< int >(
      std::max< std::int64_t >( 0, std::min< std::int64_t >( PollInterval.count( ), remaining.count( ) ) ) );

    pollfd descriptors[ 2 ] = {
      {outputClosed ? -1 : outputRead.Get( ),       POLLIN | POLLHUP, 0},
      {execErrorClosed ? -1 : execErrorRead.Get( ), POLLIN | POLLHUP, 0},
    };

    const int pollResult = poll( descriptors, 2, timeout );
    if ( pollResult < 0 && errno != EINTR )
    {
      const int pollError = errno;
      std::string cleanupError;
      TerminateAndReap( pid, processStatus, cleanupError );
      result.status = ProcessStatus::RunnerError;
      result.errorMessage = "poll failed: " + ErrorMessage( pollError );
      AppendError( result.errorMessage, cleanupError );
      break;
    }
  }

  std::string cleanupError;
  if ( outputRead && !DrainAfterTermination( outputRead.Get( ), output, outputClosed, cleanupError ) )
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

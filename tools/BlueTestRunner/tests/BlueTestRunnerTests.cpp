#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

#include "BoundedOutputBuffer.h"
#include "ProcessRunner.h"
#include "RunnerOptions.h"

#include <gtest/gtest.h>

namespace
{
using namespace BlueTestRunner;

ParsedArguments Parse( const std::initializer_list< std::string_view > arguments )
{
  const std::vector< std::string_view > values( arguments );
  return ParseArguments( values );
}

struct ShellInvocation
{
  std::filesystem::path executable;
  std::vector< std::string > arguments;
};

ShellInvocation Shell( const std::string& command )
{
#if defined( _WIN32 )
  const char* comspec = std::getenv( "COMSPEC" );
  EXPECT_NE( comspec, nullptr );
  return {
    .executable = comspec != nullptr ? std::filesystem::path( comspec ) : std::filesystem::path( "cmd.exe" ),
    .arguments = { "/d", "/s", "/c", command },
  };
#else
  return {
    .executable = "/bin/sh",
    .arguments = { "-c", command },
  };
#endif
}

ProcessResult RunShell( const std::string& command, const ProcessOptions& options = ProcessOptions{ } )
{
  const ShellInvocation shell = Shell( command );
  return RunProcess( shell.executable, shell.arguments, options );
}

TEST( BlueTestRunnerOptionsTests, ParsesExecutionLimits )
{
  const ParsedArguments parsed = Parse( { "--jobs=3", "--timeout=500ms", "--max-output=64k", "test-binary" } );

  ASSERT_TRUE( parsed.success );
  EXPECT_EQ( parsed.options.mode, JobMode::Fixed );
  EXPECT_EQ( parsed.options.requestedJobCount, 3U );
  EXPECT_EQ( parsed.options.timeout, std::chrono::milliseconds( 500 ) );
  EXPECT_EQ( parsed.options.maximumOutputBytes, 64U * 1024U );
  ASSERT_EQ( parsed.testExecutables.size( ), 1U );
  EXPECT_EQ( parsed.testExecutables.front( ), std::filesystem::path( "test-binary" ) );
}

TEST( BlueTestRunnerOptionsTests, ParsesSupportedDurationAndSizeUnits )
{
  EXPECT_EQ( Parse( { "--timeout=2m", "test" } ).options.timeout, std::chrono::minutes( 2 ) );
  EXPECT_EQ( Parse( { "--timeout=30s", "test" } ).options.timeout, std::chrono::seconds( 30 ) );
  EXPECT_EQ( Parse( { "--max-output=2M", "test" } ).options.maximumOutputBytes, 2U * 1024U * 1024U );
  EXPECT_EQ( Parse( { "--max-output=1g", "test" } ).options.maximumOutputBytes,
             static_cast< std::size_t >( 1024ULL * 1024ULL * 1024ULL ) );
}

TEST( BlueTestRunnerOptionsTests, RejectsInvalidExecutionLimits )
{
  for ( const std::string_view argument :
        { "--timeout=0s", "--timeout=10", "--timeout=1441m", "--max-output=0", "--max-output=2g", "--max-output=1t" } )
  {
    const ParsedArguments parsed = Parse( { argument, "test" } );
    EXPECT_FALSE( parsed.success ) << argument;
  }
}

TEST( BlueTestRunnerOutputTests, PreservesOutputWithinLimit )
{
  BoundedOutputBuffer output( 32 );
  output.Append( "hello" );
  output.Append( " world" );

  EXPECT_FALSE( output.Truncated( ) );
  EXPECT_EQ( output.ProducedBytes( ), 11U );
  EXPECT_EQ( output.BuildOutput( ), "hello world" );
}

TEST( BlueTestRunnerOutputTests, PreservesHeadAndTailWhenTruncated )
{
  BoundedOutputBuffer output( 8 );
  output.Append( "abcdefghijklmno" );

  const std::string captured = output.BuildOutput( );
  EXPECT_TRUE( output.Truncated( ) );
  EXPECT_EQ( output.ProducedBytes( ), 15U );
  EXPECT_TRUE( captured.starts_with( "abcd" ) );
  EXPECT_TRUE( captured.ends_with( "lmno" ) );
  EXPECT_NE( captured.find( "Captured output truncated" ), std::string::npos );
}

TEST( BlueTestRunnerProcessTests, ReportsPassingAndFailingExitCodes )
{
#if defined( _WIN32 )
  const ProcessResult passing = RunShell( "exit /b 0" );
  const ProcessResult failing = RunShell( "exit /b 7" );
#else
  const ProcessResult passing = RunShell( "exit 0" );
  const ProcessResult failing = RunShell( "exit 7" );
#endif

  EXPECT_EQ( passing.status, ProcessStatus::Exited );
  EXPECT_EQ( passing.exitCode, 0 );
  EXPECT_EQ( failing.status, ProcessStatus::Exited );
  EXPECT_EQ( failing.exitCode, 7 );
}

TEST( BlueTestRunnerProcessTests, TerminatesTimedOutProcess )
{
#if defined( _WIN32 )
  const std::string command = "ping -n 6 127.0.0.1 >nul";
#else
  const std::string command = "sleep 2";
#endif

  const ProcessResult result = RunShell( command,
                                         ProcessOptions{
                                           .timeout = std::chrono::milliseconds( 50 ),
                                           .maximumOutputBytes = 1024,
                                         } );

  EXPECT_EQ( result.status, ProcessStatus::TimedOut );
  EXPECT_EQ( result.exitCode, 124 );
}

TEST( BlueTestRunnerProcessTests, BoundsCapturedOutput )
{
#if defined( _WIN32 )
  const std::string command = "for /L %i in (1,1,200) do @echo 0123456789abcdef";
#else
  const std::string command = "i=0; while [ $i -lt 200 ]; do printf '0123456789abcdef\\n'; i=$((i+1)); done";
#endif

  const ProcessResult result = RunShell( command,
                                         ProcessOptions{
                                           .timeout = std::chrono::seconds( 5 ),
                                           .maximumOutputBytes = 128,
                                         } );

  ASSERT_EQ( result.status, ProcessStatus::Exited );
  ASSERT_EQ( result.exitCode, 0 );
  EXPECT_TRUE( result.outputTruncated );
  EXPECT_GT( result.producedOutputBytes, 128U );
  EXPECT_NE( result.output.find( "Captured output truncated" ), std::string::npos );
}


#if !defined( _WIN32 )
TEST( BlueTestRunnerProcessTests, HandlesClosedOutputWithoutBusyPollDependency )
{
  const auto startTime = std::chrono::steady_clock::now( );
  const ProcessResult result = RunShell( "exec >/dev/null 2>&1; sleep 1",
                                         ProcessOptions{
                                           .timeout = std::chrono::seconds( 3 ),
                                           .maximumOutputBytes = 1024,
                                         } );
  const auto elapsed = std::chrono::steady_clock::now( ) - startTime;

  EXPECT_EQ( result.status, ProcessStatus::Exited );
  EXPECT_EQ( result.exitCode, 0 );
  EXPECT_GE( elapsed, std::chrono::milliseconds( 500 ) );
  EXPECT_LT( elapsed, std::chrono::milliseconds( 2500 ) );
}

TEST( BlueTestRunnerProcessTests, SustainedOutputStillHonorsTimeout )
{
  const ProcessResult result = RunShell( "while :; do printf '0123456789abcdef0123456789abcdef\\n'; done",
                                         ProcessOptions{
                                           .timeout = std::chrono::milliseconds( 50 ),
                                           .maximumOutputBytes = 128,
                                         } );

  EXPECT_EQ( result.status, ProcessStatus::TimedOut );
  EXPECT_EQ( result.exitCode, 124 );
  EXPECT_TRUE( result.outputTruncated );
  EXPECT_GT( result.producedOutputBytes, 128U );
}
#endif

TEST( BlueTestRunnerProcessTests, ReportsLaunchFailure )
{
  const ProcessResult result =
    RunProcess( std::filesystem::path( "definitely-not-a-real-blue-test-executable" ), { }, ProcessOptions{ } );

  EXPECT_EQ( result.status, ProcessStatus::LaunchFailed );
  EXPECT_FALSE( result.errorMessage.empty( ) );
}
} // namespace

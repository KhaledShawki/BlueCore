#pragma once

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace BlueTestRunner
{
enum class JobMode
{
  Sequential,
  Fixed,
  Auto,
};

enum class ProcessStatus
{
  Exited,
  TimedOut,
  LaunchFailed,
  RunnerError,
};

enum class TestStatus
{
  Passed,
  Failed,
  TimedOut,
  Missing,
  LaunchFailed,
  RunnerError,
};

struct RunnerOptions
{
  JobMode mode = JobMode::Auto;
  std::size_t requestedJobCount = 0;
  std::chrono::milliseconds timeout{ 120000 };
  std::size_t maximumOutputBytes = 8U * 1024U * 1024U;
  bool listOnly = false;
  bool help = false;
};

struct ParsedArguments
{
  RunnerOptions options;
  std::vector< std::filesystem::path > testExecutables;
  bool success = true;
  int exitCode = 0;
  std::string errorMessage;
};

struct ProcessOptions
{
  std::chrono::milliseconds timeout{ 120000 };
  std::size_t maximumOutputBytes = 8U * 1024U * 1024U;
};

struct ProcessResult
{
  ProcessStatus status = ProcessStatus::RunnerError;
  int exitCode = 1;
  std::string output;
  std::uint64_t producedOutputBytes = 0;
  bool outputTruncated = false;
  std::string errorMessage;
};

struct TestResult
{
  std::filesystem::path executablePath;
  TestStatus status = TestStatus::RunnerError;
  int exitCode = 1;
  double elapsedMilliseconds = 0.0;
  std::string output;
  std::uint64_t producedOutputBytes = 0;
  bool outputTruncated = false;
  std::string errorMessage;
};

struct RunSummary
{
  std::vector< TestResult > results;
  double wallElapsedMilliseconds = 0.0;
  std::size_t workerCount = 1;
};
} // namespace BlueTestRunner

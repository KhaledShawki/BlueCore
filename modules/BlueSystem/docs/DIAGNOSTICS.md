# Diagnostics

BlueSystem keeps debugger and assertion APIs behind platform-neutral headers.

## Debug API

```cpp
namespace Blue
{
	Bool IsDebuggerAttached() noexcept;
	void BreakIntoDebugger() noexcept;
	void WriteDebugOutput(const Char* message) noexcept;
}
```

`BLUE_DEBUG_BREAK()` remains available for call sites that need a macro.

## Platform behavior

Windows uses `IsDebuggerPresent`, `__debugbreak`, and `OutputDebugStringA`.

Linux reads `/proc/self/status` for `TracerPid`, uses `SIGTRAP`, and writes fallback diagnostics to `stderr`.

macOS uses `sysctl` with `KERN_PROC_PID`, uses `SIGTRAP`, and writes fallback diagnostics to `stderr`.

## Assertions

Assertion reporting follows this order:

1. Invoke a registered custom assert handler, if one exists.
2. Write a detailed report to debug output.
3. If the logger is initialized and has sinks, emit an assertion log event.

The assertion path must not allocate memory.

## Source location

`BLUE_SOURCE_LOCATION()` captures file, line, and function information through the centralized compiler abstraction.

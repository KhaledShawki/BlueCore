# BlueSystem

BlueSystem is the lowest Blue runtime module. It provides platform-neutral base facilities used by the rest of the framework.

## Responsibilities

- Base integer, character, and boolean types
- Compiler, platform, and architecture detection
- Assertions and source location support
- Debugger-facing diagnostics
- Monotonic time primitives
- Processor queries and pause/yield helpers
- Native atomics and spin locks
- Native threads and thread policy APIs
- Blocking synchronization primitives
- Low-allocation logging infrastructure

## Public header policy

Public BlueSystem headers do not expose native platform headers such as `windows.h` or `pthread.h`. Platform details live in `src/Platform/<OS>/`.

## System docs

| Document | Topic |
|---|---|
| `docs/ATOMICS.md` | Atomic primitives and spin locks. |
| `docs/THREADING.md` | Native thread API, ownership, priority, and affinity policy. |
| `docs/SYNCHRONIZATION.md` | Mutex, semaphore, event, and condition-variable behavior. |
| `docs/LOGGING.md` | Logger categories, sinks, filtering, and recursion guard. |
| `docs/DIAGNOSTICS.md` | Assertions, debug output, and debugger integration. |
| `docs/TIME.md` | Time points, durations, stopwatch, and platform clocks. |
| `docs/PROCESSOR.md` | Processor architecture, logical processor count, and cache line size. |

## Logging example

```cpp
BLUE_DEFINE_LOG_CATEGORY(LogSystem, Blue::LogLevel::Info);

int main()
{
	Blue::InitializeLogger();
	Blue::RegisterLogSink(Blue::CreateConsoleLogSink());

	BLUE_LOG_INFO(LogSystem, "BlueSystem logger initialized");

	Blue::ShutdownLogger();
}
```

## Synchronization example

```cpp
Blue::OwnedMutex mutex;

if (mutex.IsValid())
{
	Blue::ScopedMutexLock lock(mutex.Get());
	// protected work
}
```

Use `SpinLock` only for very short critical sections. Use `Mutex` or owned synchronization wrappers for blocking or potentially contended work.

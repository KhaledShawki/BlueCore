# Logging

BlueSystem provides a low-allocation logger suitable for runtime diagnostics.

## Public API

```cpp
Blue::InitializeLogger();
Blue::RegisterLogSink(Blue::CreateConsoleLogSink());
BLUE_LOG_INFO(LogCategoryName, "message");
Blue::FlushLogger();
Blue::ShutdownLogger();
```

## Categories

Define categories in source files:

```cpp
BLUE_DEFINE_LOG_CATEGORY(LogSystem, Blue::LogLevel::Info);
```

Use inline category definitions only when a category must be defined in a header:

```cpp
BLUE_DEFINE_INLINE_LOG_CATEGORY(LogMemory, Blue::LogLevel::Debug);
```

## Thread safety

The logger protects the sink registry with `SpinLock`, but it does not hold the lock while invoking sink callbacks. This keeps slow sinks from extending the critical section.

## Sink lifetime

Registered sink contexts must remain valid while logging can occur. Do not destroy sink-owned state concurrently with active logging. Coordinate shutdown before destroying external sink resources.

## Recursion guard

If a sink attempts to log while handling a log event, the recursive event is dropped and the dropped counter is incremented. This prevents infinite recursion in diagnostic failure paths.

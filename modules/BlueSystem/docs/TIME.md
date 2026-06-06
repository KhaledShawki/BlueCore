# Time

BlueSystem provides monotonic timing primitives for elapsed-time measurement, diagnostics, profiling, and scheduling support.

## Types

- `TimePoint`
- `TimeDuration`
- `Stopwatch`

## Helpers

The API includes nanosecond, microsecond, millisecond, and second conversion helpers, elapsed-time helpers, comparison operators, and arithmetic operators.

Elapsed subtraction saturates to zero to avoid unsigned underflow. Large duration conversions and arithmetic are guarded against overflow.

## Platform clocks

Windows uses `QueryPerformanceCounter` and `QueryPerformanceFrequency`.

Linux and macOS use `clock_gettime(CLOCK_MONOTONIC)`.

## Non-goals

This layer does not provide calendar time, wall-clock time, time zones, sleep scheduling, or profiler event recording.

# Synchronization

BlueSystem provides native synchronization primitives with explicit low-level APIs and owned wrappers.

## Primitives

- `Mutex`
- `Semaphore`
- `Event`
- `ConditionVariable`

## Ownership models

Low-level primitives use explicit initialization and shutdown:

```cpp
Blue::Mutex mutex;
Blue::InitializeMutex(mutex);

mutex.Acquire();
mutex.Release();

Blue::ShutdownMutex(mutex);
```

Owned wrappers initialize in their constructors and shut down in their destructors:

```cpp
Blue::OwnedMutex mutex;

if (mutex.IsValid())
{
	Blue::ScopedMutexLock lock(mutex.Get());
	// protected work
}
```

Owned wrappers are non-copyable and expose `IsValid()` for no-exception construction failure handling.

## Naming

Lock-like primitives use these operation names:

```text
Acquire
TryAcquire
Release
```

Event primitives use signal/reset/wait names:

```text
Signal
Reset
Wait
TryWait
```

## Timed waits

Timed waits are available for primitives that can reasonably timeout:

```cpp
semaphore.AcquireFor(timeout);
event.WaitFor(timeout);
conditionVariable.WaitFor(mutex, timeout);
```

A timeout returns `false`. A successful acquire or wake returns `true`.

## Condition variables

Condition variables may wake spuriously. Always wait in a predicate loop:

```cpp
mutex.Acquire();
while (!predicate)
{
	conditionVariable.Wait(mutex);
}
mutex.Release();
```

## Platform backends

Windows uses SRW locks, condition variables, events, semaphores, and wait APIs.

Linux and macOS share the pthread-backed POSIX synchronization layer where behavior is common.

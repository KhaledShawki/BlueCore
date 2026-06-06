# Threading

BlueSystem provides a platform-neutral native thread API with explicit ownership.

## Thread creation

```cpp
Blue::Thread thread;

Blue::ThreadCreateInfo info;
info.Name = "Worker";
info.Entry = &WorkerEntry;
info.UserData = &context;
info.StackSize = 0;
info.Priority = Blue::ThreadPriority::Normal;
info.Affinity = Blue::CpuAffinity::Any();

if (Blue::CreateThread(thread, info))
{
	Blue::Uint32 exitCode = 0;
	Blue::JoinThread(thread, &exitCode);
}
```

`ThreadCreateDesc` remains available as a compatibility alias for `ThreadCreateInfo`.

## Ownership

`CreateThread()` creates a joinable native thread handle.

`JoinThread()` consumes the handle.

`DetachThread()` consumes the handle.

The raw `Thread` primitive has no destructor. `OwnedThread` is the RAII wrapper for code that should not manually manage handle lifetime. Its destructor detaches a still-joinable handle as a last-resort cleanup path and does not block.

## Priority and affinity

The API exposes explicit success/failure returns:

```cpp
SetThreadPriority(thread, Blue::ThreadPriority::High);
SetCurrentThreadPriority(Blue::ThreadPriority::High);
SetThreadAffinity(thread, Blue::CpuAffinity::FromProcessorIndex(0));
SetCurrentThreadAffinity(Blue::CpuAffinity::FromProcessorIndex(0));
```

Priority and affinity are best-effort. Operating-system permissions and capabilities may reject a request.

## Platform backends

Windows:

```text
_beginthreadex
WaitForSingleObject
CloseHandle
SetThreadDescription
SetThreadPriority
SetThreadAffinityMask
```

Linux:

```text
pthread_create
pthread_join
pthread_detach
pthread_setschedparam
pthread_setaffinity_np
pthread_setname_np
```

macOS:

```text
pthread_create
pthread_join
pthread_detach
pthread_setschedparam
pthread_setname_np
```

macOS does not expose a portable hard CPU affinity API, so enabled affinity requests return `false` there.

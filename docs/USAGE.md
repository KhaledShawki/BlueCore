# Usage

This page shows a minimal Blue usage pattern.

## Include headers

```cpp
#include <Blue/System/Types.h>
#include <Blue/System/Log/Logger.h>
#include <Blue/Memory/MemorySystem.h>
#include <Blue/Container/SmidString.h>
```

## Initialize memory

```cpp
Blue::MemorySystemDesc memoryDesc = { };
memoryDesc.EnableMetrics = true;
memoryDesc.EnableTracking = true;
memoryDesc.EnableLeakDetection = true;

Blue::InitializeMemorySystem(memoryDesc);
```

## Use allocator-aware types

```cpp
Blue::Allocator allocator = Blue::GetDefaultAllocator();
Blue::SmidString name("Blue", allocator);
```

## Shutdown

```cpp
Blue::ShutdownMemorySystem();
```

Initialize systems before use and shut them down in reverse order of ownership.

# Usage

This document shows a minimal example of how to initialize and use BlueCore.

## Include Headers

```cpp
#include <Blue/System/Types.h>
#include <Blue/System/Log/Logger.h>
#include <Blue/Memory/MemorySystem.h>
#include <Blue/Container/SmidString.h>
```

## Initialize the Memory System

Before using most BlueCore functionality, the memory system must be initialized.

```cpp
Blue::MemorySystemDesc memoryDesc = {};
memoryDesc.EnableMetrics = true;
memoryDesc.EnableTracking = true;
memoryDesc.EnableLeakDetection = true;

Blue::InitializeMemorySystem(memoryDesc);
```

## Using Allocator-Aware Types

Once the memory system is initialized, you can use allocator-aware types:

```cpp
Blue::Allocator allocator = Blue::GetDefaultAllocator();
Blue::SmidString name("BlueCore", allocator);
```

## Shutdown

When the application is shutting down, the memory system should be shut down as well:

```cpp
Blue::ShutdownMemorySystem();
```

Systems should be initialized and shut down in reverse order of their dependencies. The memory system is typically one of the first systems to initialize and one of the last to shut down.
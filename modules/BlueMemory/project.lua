bb.module {
    name = "BlueMemory",
    type = "library",
    linkage = "auto",
    root = "modules/BlueMemory",

    files = {
        public_headers = {
            "include/Blue/Memory.h",
            "include/Blue/Memory/Allocation/AllocationFreeRequest.h",
            "include/Blue/Memory/Allocation/AllocationValidation.h",
            "include/Blue/Memory/AllocationFailureInfo.h",
            "include/Blue/Memory/AllocationFailurePolicy.h",
            "include/Blue/Memory/AllocationFailureReason.h",
            "include/Blue/Memory/AllocationFlags.h",
            "include/Blue/Memory/AllocationTag.h",
            "include/Blue/Memory/Allocator.h",
            "include/Blue/Memory/AllocatorInvoker.h",
            "include/Blue/Memory/AllocatorKind.h",
            "include/Blue/Memory/Api.h",
            "include/Blue/Memory/Backend/SystemMemoryBackend.h",
            "include/Blue/Memory/BlueNew.h",
            "include/Blue/Memory/HeapAllocator.h",
            "include/Blue/Memory/Invoker/MemoryNewInvoker.h",
            "include/Blue/Memory/Invoker/RuntimeAllocationInvoker.h",
            "include/Blue/Memory/LinearAllocator.h",
            "include/Blue/Memory/MemoryBlock.h",
            "include/Blue/Memory/MemoryMetrics.h",
            "include/Blue/Memory/MemoryMetricsMode.h",
            "include/Blue/Memory/MemorySystem.h",
            "include/Blue/Memory/MemoryUnits.h",
            "include/Blue/Memory/Metrics/MemoryThreadContext.h",
            "include/Blue/Memory/Metrics/MetricsProxy.h",
            "include/Blue/Memory/Oom/OomReport.h",
            "include/Blue/Memory/Oom/OomReporter.h",
            "include/Blue/Memory/Pool/MemoryPoolDesc.h",
            "include/Blue/Memory/Pool/MemoryPoolId.h",
            "include/Blue/Memory/Pool/MemoryPoolPolicy.h",
            "include/Blue/Memory/Pool/MemoryPoolRegistry.h",
            "include/Blue/Memory/Pool/MemoryPoolResolver.h",
            "include/Blue/Memory/Pool/MemoryPoolState.h",
            "include/Blue/Memory/Pool/MemoryPoolStats.h",
            "include/Blue/Memory/Pool/MemoryPoolTrait.h",
            "include/Blue/Memory/PoolAllocator.h",
            "include/Blue/Memory/Proxy/AllocatorProxy.h",
            "include/Blue/Memory/Proxy/RuntimeAllocationProxy.h",
            "include/Blue/Memory/Proxy/TypedAllocationProxy.h",
            "include/Blue/Memory/UniquePtr.h",
            "include/Blue/Memory/VirtualMemoryBlock.h",
            "include/BlueMemory/BlueMemory.h",
            "include/BlueMemory/BlueMemoryApi.h",

        },

        resources = {
            "include/Blue/Memory/Pool/MemoryPools.def",

        },

        private_headers = {
            "src/BlueMemoryPrivate.h",
            "src/Pch.h",

        },

        sources = {
            "src/AllocationFailure.cpp",
            "src/AllocatorKind.cpp",
            "src/Backend/MimallocBackend.cpp",
            "src/Backend/SystemMemoryBackend.cpp",
            "src/HeapAllocator.cpp",
            "src/Invoker/RuntimeAllocationInvoker.cpp",
            "src/LinearAllocator.cpp",
            "src/MemoryMetrics.cpp",
            "src/MemorySystem.cpp",
            "src/Metrics/MemoryThreadContext.cpp",
            "src/Oom/OomReporter.cpp",
            "src/Pch.cpp",
            "src/Pool/MemoryPoolRegistry.cpp",
            "src/PoolAllocator.cpp",
            "src/Proxy/RuntimeAllocationProxy.cpp",
            "src/VirtualMemoryBlock.cpp",

        },
    },

    public_include_dirs = {
        "modules/BlueMemory/include",
    },

    public_defines = {
        "BLUE_MEMORY=1",
    },

    deps = {
        public = {
            "BlueSystem",
        },
    },

    filters = {
        {
            when = { "options:memory-backend=mimalloc" },
            deps = { "mimalloc" },
        },
    },
}

bb.module_tests {
    module = "BlueMemory",
    root = "modules/BlueMemory",

    deps = {
        "BlueMemory",
        "BlueSystem",
    },

    platform = {
        windows = {
            private_links = {
                "kernel32",
            },
        },
        linux = {
            private_links = {
                "pthread",
                "dl",
                "rt",
            },
        },
        macosx = {
            private_links = {
                "pthread",
            },
        },
    },

    tests = {
        "BlueMemoryPoolResolverTests",
        "BlueMemoryInvokerTests",
        "BlueMemoryRuntimeAllocationTests",
    },
}

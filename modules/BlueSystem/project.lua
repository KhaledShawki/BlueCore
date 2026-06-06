bb.static_library {
    name = "BlueSystem",
    root = "modules/BlueSystem",
    default_files = false,

    files = {
        "modules/BlueSystem/include/**.h",
        "modules/BlueSystem/include/**.hpp",
        "modules/BlueSystem/include/**.inl",
        "modules/BlueSystem/src/Assert.cpp",
        "modules/BlueSystem/src/Log/Logger.cpp",
        "modules/BlueSystem/src/Processor.cpp",
        "modules/BlueSystem/src/Result.cpp",
        "modules/BlueSystem/src/Time.cpp",
    },

    public_include_dirs = {
        "modules/BlueSystem/include",
    },

    public_defines = {
        "BLUE_SYSTEM=1",
    },

    platform = {
        windows = {
            files = {
                "modules/BlueSystem/src/Platform/Windows/**.cpp",
            },
            private_links = {
                "kernel32",
            },
        },
        linux = {
            files = {
                "modules/BlueSystem/src/Platform/POSIX/**.cpp",
                "modules/BlueSystem/src/Platform/Linux/**.cpp",
            },
            private_links = {
                "pthread",
                "dl",
                "rt",
                "atomic",
            },
        },
        macosx = {
            files = {
                "modules/BlueSystem/src/Platform/POSIX/**.cpp",
                "modules/BlueSystem/src/Platform/MacOS/**.cpp",
            },
            private_links = {
                "pthread",
            },
        },
    },
}


bb.module_tests {
    module = "BlueSystem",
    root = "modules/BlueSystem",

    deps = {
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
                "atomic",
            },
        },
        macosx = {
            private_links = {
                "pthread",
            },
        },
    },

    tests = {
        "BlueSystemAtomicTests",
        "BlueSystemPhase2Tests",
        "BlueSystemThreadingTests",
        "BlueSystemLoggerThreadSafetyTests",
        "BlueSystemDiagnosticsTests",
        "BlueSystemTimeTests",
        "BlueSystemProcessorTests",
        "BlueSystemMutexTests",
        "BlueSystemSemaphoreTests",
        "BlueSystemEventTests",
        "BlueSystemConditionVariableTests",
        "BlueSystemSynchronizationStressTests",
    },
}

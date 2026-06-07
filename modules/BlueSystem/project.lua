bb.module {
    name = "BlueSystem",
    type = "library",
    linkage = "auto",
    root = "modules/BlueSystem",

    files = {
        public_headers = {
            "include/Blue/System.h",
            "include/Blue/System/Alignment.h",
            "include/Blue/System/Api.h",
            "include/Blue/System/Architecture.h",
            "include/Blue/System/Assert.h",
            "include/Blue/System/Atomic.h",
            "include/Blue/System/Base/Alignment.h",
            "include/Blue/System/Base/Alignment.inl",
            "include/Blue/System/Base/NonCopyable.h",
            "include/Blue/System/Base/PrimitiveTypes.h",
            "include/Blue/System/Base/Result.h",
            "include/Blue/System/Base/Result.inl",
            "include/Blue/System/Base/SourceLocation.h",
            "include/Blue/System/Compiler.h",
            "include/Blue/System/ConditionVariable.h",
            "include/Blue/System/Debug.h",
            "include/Blue/System/Diagnostics/Debug.h",
            "include/Blue/System/Event.h",
            "include/Blue/System/Log.h",
            "include/Blue/System/Log/LogCategory.h",
            "include/Blue/System/Log/LogEvent.h",
            "include/Blue/System/Log/LogLevel.h",
            "include/Blue/System/Log/LogMacros.h",
            "include/Blue/System/Log/LogSink.h",
            "include/Blue/System/Log/Logger.h",
            "include/Blue/System/Mutex.h",
            "include/Blue/System/NonCopyable.h",
            "include/Blue/System/Platform.h",
            "include/Blue/System/Platform/WindowsLean.h",
            "include/Blue/System/Processor.h",
            "include/Blue/System/Result.h",
            "include/Blue/System/Semaphore.h",
            "include/Blue/System/SourceLocation.h",
            "include/Blue/System/SpinLock.h",
            "include/Blue/System/Thread.h",
            "include/Blue/System/Threading/Atomic.h",
            "include/Blue/System/Threading/Atomic.inl",
            "include/Blue/System/Threading/Atomic_GccClang.inl",
            "include/Blue/System/Threading/Atomic_Msvc.inl",
            "include/Blue/System/Threading/Atomic_Primitives.h",
            "include/Blue/System/Threading/ConditionVariable.h",
            "include/Blue/System/Threading/ConditionVariable.inl",
            "include/Blue/System/Threading/Event.h",
            "include/Blue/System/Threading/Event.inl",
            "include/Blue/System/Threading/Mutex.h",
            "include/Blue/System/Threading/Mutex.inl",
            "include/Blue/System/Threading/Processor.h",
            "include/Blue/System/Threading/Processor.inl",
            "include/Blue/System/Threading/Processor_GccClang.inl",
            "include/Blue/System/Threading/Processor_Msvc.inl",
            "include/Blue/System/Threading/Semaphore.h",
            "include/Blue/System/Threading/Semaphore.inl",
            "include/Blue/System/Threading/SpinLock.h",
            "include/Blue/System/Threading/SpinLock.inl",
            "include/Blue/System/Threading/Thread.h",
            "include/Blue/System/Threading/Thread.inl",
            "include/Blue/System/Threading/ThreadTypes.h",
            "include/Blue/System/Time.h",
            "include/Blue/System/Time.inl",
            "include/Blue/System/Types.h",
            "include/BlueSystem/BlueSystem.h",
            "include/BlueSystem/BlueSystemApi.h",

        },

        private_headers = {
            "src/BlueSystemPrivate.h",
            "src/Pch.h",

        },

        sources = {
            "src/Assert.cpp",
            "src/Log/Logger.cpp",
            "src/Pch.cpp",
            "src/Processor.cpp",
            "src/Result.cpp",
            "src/Time.cpp",

        },

        platform = {
            windows = {
                private_headers = {
                    "src/Platform/Windows/Windows_Synchronization.h",

                },

                sources = {
                    "src/Platform/Windows/ConditionVariable_Windows.cpp",
                    "src/Platform/Windows/Debug_Windows.cpp",
                    "src/Platform/Windows/Event_Windows.cpp",
                    "src/Platform/Windows/Mutex_Windows.cpp",
                    "src/Platform/Windows/Processor_Windows.cpp",
                    "src/Platform/Windows/Semaphore_Windows.cpp",
                    "src/Platform/Windows/Thread_Windows.cpp",

                },
            },

            linux = {
                private_headers = {
                    "src/Platform/POSIX/POSIX_Synchronization.h",
                    "src/Platform/POSIX/POSIX_Thread.h",

                },

                sources = {
                    "src/Platform/Linux/Debug_Linux.cpp",
                    "src/Platform/Linux/Processor_Linux.cpp",
                    "src/Platform/Linux/Thread_Linux.cpp",
                    "src/Platform/POSIX/ConditionVariable_POSIX.cpp",
                    "src/Platform/POSIX/Event_POSIX.cpp",
                    "src/Platform/POSIX/Mutex_POSIX.cpp",
                    "src/Platform/POSIX/POSIX_Synchronization.cpp",
                    "src/Platform/POSIX/Semaphore_POSIX.cpp",
                    "src/Platform/POSIX/Thread_POSIX.cpp",

                },
            },

            macosx = {
                private_headers = {
                    "src/Platform/POSIX/POSIX_Synchronization.h",
                    "src/Platform/POSIX/POSIX_Thread.h",

                },

                sources = {
                    "src/Platform/MacOS/Debug_MacOS.cpp",
                    "src/Platform/MacOS/Processor_MacOS.cpp",
                    "src/Platform/MacOS/Thread_MacOS.cpp",
                    "src/Platform/POSIX/ConditionVariable_POSIX.cpp",
                    "src/Platform/POSIX/Event_POSIX.cpp",
                    "src/Platform/POSIX/Mutex_POSIX.cpp",
                    "src/Platform/POSIX/POSIX_Synchronization.cpp",
                    "src/Platform/POSIX/Semaphore_POSIX.cpp",
                    "src/Platform/POSIX/Thread_POSIX.cpp",

                },
            },
        },
    },

    public_include_dirs = {
        "modules/BlueSystem/include",
    },

    public_defines = {
        "BLUE_SYSTEM=1",
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

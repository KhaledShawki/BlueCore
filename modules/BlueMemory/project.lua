bb.static_library {
    name = "BlueMemory",
    root = "modules/BlueMemory",

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

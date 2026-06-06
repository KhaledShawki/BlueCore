bb.dependency {
    name = "mimalloc",
    kind = "External",
    version = "3.x",

    external_include_dirs = {
        interface = {
            "third_party/mimalloc/include",
        },
    },

    lib_dirs = {
        interface = {
            "third_party/mimalloc/lib/%{cfg.system}/%{cfg.architecture}/%{cfg.buildcfg}",
        },
    },

    links = {
        interface = {
            "mimalloc",
        },
    },

    filters = {
        {
            when = { "options:memory-backend=system" },
            links = {},
            lib_dirs = {},
            external_include_dirs = {},
        },
    },
}

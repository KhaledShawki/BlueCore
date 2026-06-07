bb.module {
    name = "BlueBenchmarks",
    type = "executable",
    root = "apps/BlueBenchmarks",

    files = {
        private_headers = {
            "src/Pch.h",
        },

        sources = {
            "src/Main.cpp",
            "src/Pch.cpp",
        },
    },

    private_include_dirs = {
        "apps/BlueBenchmarks/src",
    },

    deps = {
        private = {
            "BlueSystem",
            "BlueMemory",
            "BlueContainer",
        },
    },
}

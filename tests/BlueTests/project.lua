bb.module({
    name = "BlueTests",
    type = "executable",
    root = "tests/BlueTests",

    files = {
        private_headers = {
            "src/Pch.h",
            "src/TestFramework.h",
        },

        sources = {
            "src/Main.cpp",
            "src/Pch.cpp",
        },
    },

    private_include_dirs = {
        "tests/BlueTests/src",
    },

    deps = {
        private = {
            "BlueSystem",
            "BlueMemory",
            "BlueContainer",
            "BlueJobSystem",
        },
    },
})

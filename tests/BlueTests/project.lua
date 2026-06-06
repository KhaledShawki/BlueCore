bb.executable {
    name = "BlueTests",
    root = "tests/BlueTests",

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
}

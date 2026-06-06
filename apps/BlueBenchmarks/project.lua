bb.executable {
    name = "BlueBenchmarks",
    root = "apps/BlueBenchmarks",

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

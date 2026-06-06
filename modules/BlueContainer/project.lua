bb.static_library {
    name = "BlueContainer",
    root = "modules/BlueContainer",

    public_include_dirs = {
        "modules/BlueContainer/include",
    },

    public_defines = {
        "BLUE_CONTAINER=1",
    },

    deps = {
        public = {
            "BlueSystem",
            "BlueMemory",
        },
    },
}

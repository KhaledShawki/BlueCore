bb.static_library {
    name = "BlueJobSystem",
    root = "modules/BlueJobSystem",

    public_include_dirs = {
        "modules/BlueJobSystem/include",
    },

    public_defines = {
        "BLUE_JOB_SYSTEM=1",
    },

    deps = {
        public = {
            "BlueSystem",
            "BlueMemory",
            "BlueContainer",
        },
    },
}

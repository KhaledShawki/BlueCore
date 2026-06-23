bb.module({
    name = "BlueJobSystem",
    type = "library",
    linkage = "auto",
    root = "modules/BlueJobSystem",

    files = {
        public_headers = {
            "include/Blue/JobSystem.h",
            "include/Blue/JobSystem/Api.h",
            "include/Blue/JobSystem/JobSystem.h",
            "include/BlueJobSystem/BlueJobSystem.h",
            "include/BlueJobSystem/BlueJobSystemApi.h",
        },

        private_headers = {
            "src/BlueJobSystemPrivate.h",
            "src/Pch.h",
        },

        sources = {
            "src/JobSystem.cpp",
            "src/Pch.cpp",
        },
    },

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
})

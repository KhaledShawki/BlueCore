bb.module({
    name = "BlueContainer",
    type = "library",
    linkage = "auto",
    root = "modules/BlueContainer",

    files = {
        public_headers = {
            "include/Blue/Container.h",
            "include/Blue/Container/Api.h",
            "include/Blue/Container/DynamicArray.h",
            "include/Blue/Container/RingBuffer.h",
            "include/Blue/Container/SmidString.h",
            "include/Blue/Container/Span.h",
            "include/Blue/Container/StringView.h",
            "include/BlueContainer/BlueContainer.h",
            "include/BlueContainer/BlueContainerApi.h",
        },

        private_headers = {
            "src/BlueContainerPrivate.h",
            "src/Pch.h",
        },

        sources = {
            "src/Pch.cpp",
            "src/SmidString.cpp",
        },
    },

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
})

local googletestRoot = "third_party/googletest/googletest"

bb.module {
    name = "gtest",
    type = "library",
    linkage = "static",
    root = googletestRoot,
    default_files = false,
    pch = false,

    files = {
        path.join(googletestRoot, "include/gtest/**.h"),
        path.join(googletestRoot, "src/*.h"),
        path.join(googletestRoot, "src/gtest-all.cc"),
    },

    public_include_dirs = {
        path.join(googletestRoot, "include"),
    },

    private_include_dirs = {
        googletestRoot,
    },

}

bb.module {
    name = "gtest_main",
    type = "library",
    linkage = "static",
    root = googletestRoot,
    default_files = false,
    pch = false,

    files = {
        path.join(googletestRoot, "src/gtest_main.cc"),
    },

    public_include_dirs = {
        path.join(googletestRoot, "include"),
    },

    private_include_dirs = {
        googletestRoot,
    },

    deps = {
        public = {
            "gtest",
        },
    },
}

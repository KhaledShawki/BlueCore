local mimallocRoot = "third_party/mimalloc"

if (_OPTIONS["memory-backend"] or "system") ~= "mimalloc" then
    return
end

if not os.isfile(path.join(BLUE_ROOT, mimallocRoot, "include/mimalloc.h")) then
    error("mimalloc submodule is missing. Run: git submodule update --init --recursive third_party/mimalloc")
end

local mimallocSources = {
    "src/alloc.c",
    "src/alloc-aligned.c",
    "src/alloc-posix.c",
    "src/arena.c",
    "src/arena-meta.c",
    "src/bitmap.c",
    "src/heap.c",
    "src/init.c",
    "src/libc.c",
    "src/options.c",
    "src/os.c",
    "src/page.c",
    "src/page-map.c",
    "src/random.c",
    "src/stats.c",
    "src/theap.c",
    "src/threadlocal.c",
    "src/prim/prim.c",
}

local mimallocFiles = {
    path.join(mimallocRoot, "include/mimalloc.h"),
    path.join(mimallocRoot, "include/mimalloc-override.h"),
    path.join(mimallocRoot, "include/mimalloc-new-delete.h"),
    path.join(mimallocRoot, "include/mimalloc-stats.h"),
    path.join(mimallocRoot, "include/mimalloc/**.h"),
    path.join(mimallocRoot, "src/**.h"),
}

for _, source in ipairs(mimallocSources) do
    table.insert(mimallocFiles, path.join(mimallocRoot, source))
end

bb.module {
    name = "mimalloc",
    type = "library",
    linkage = "static",
    root = mimallocRoot,
    default_files = false,
    pch = false,

    files = mimallocFiles,

    external_include_dirs = {
        public = {
            path.join(mimallocRoot, "include"),
        },
    },

    private_include_dirs = {
        path.join(mimallocRoot, "include"),
        path.join(mimallocRoot, "src"),
    },

    private_defines = {
        "MI_STATIC_LIB=1",
    },

    build_options = {
        private = {
            "-w",
        },
    },

    platform = {
        windows = {
            build_options = {
                "/w",
            },
        },
    },

    filters = {
        {
            when = { "system:windows" },
            links = {
                "psapi",
                "shell32",
                "user32",
                "advapi32",
                "bcrypt",
            },
        },
        {
            when = { "system:linux" },
            links = {
                "pthread",
                "rt",
            },
        },
        {
            when = { "system:macosx" },
            links = {
                "pthread",
            },
        },
    },
}

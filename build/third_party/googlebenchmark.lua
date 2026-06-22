local benchmarkRoot = "third_party/benchmark"

if not os.isfile(path.join(BLUE_ROOT, benchmarkRoot, "include/benchmark/benchmark.h")) then
    error("Google Benchmark submodule is missing. Run: git submodule update --init --recursive third_party/googlebenchmark")
end

local benchmarkSources = {
    "src/benchmark.cc",
    "src/benchmark_api_internal.cc",
    "src/benchmark_name.cc",
    "src/benchmark_register.cc",
    "src/benchmark_runner.cc",
    "src/check.cc",
    "src/colorprint.cc",
    "src/commandlineflags.cc",
    "src/complexity.cc",
    "src/console_reporter.cc",
    "src/counter.cc",
    "src/csv_reporter.cc",
    "src/json_reporter.cc",
    "src/perf_counters.cc",
    "src/reporter.cc",
    "src/sleep.cc",
    "src/statistics.cc",
    "src/string_util.cc",
    "src/sysinfo.cc",
    "src/timers.cc",
}

local benchmarkFiles = {
    path.join(benchmarkRoot, "include/benchmark/benchmark.h"),
    path.join(benchmarkRoot, "src/*.h"),
}

for _, source in ipairs(benchmarkSources) do
    table.insert(benchmarkFiles, path.join(benchmarkRoot, source))
end

bb.module {
    name = "benchmark",
    type = "library",
    linkage = "static",
    root = benchmarkRoot,
    default_files = false,
    pch = false,

    files = benchmarkFiles,

    external_include_dirs = {
        public = {
            path.join(benchmarkRoot, "include"),
        },
    },

    private_include_dirs = {
        benchmarkRoot,
        path.join(benchmarkRoot, "src"),
        path.join(benchmarkRoot, "include"),
    },

    public_defines = {
        "BENCHMARK_STATIC_DEFINE=1",
    },

    private_defines = {
        "BENCHMARK_STATIC_DEFINE=1",
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
            private_links = {
                "shlwapi",
            },
        },
        linux = {
            private_links = {
                "pthread",
                "rt",
            },
        },
        macosx = {
            private_links = {
                "pthread",
            },
        },
    },
}
